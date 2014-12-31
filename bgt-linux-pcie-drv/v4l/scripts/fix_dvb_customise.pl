#!/usr/bin/perl
use strict;
use warnings;
use File::Find;
use Fcntl ':mode';

my $debug = 0;

my $SRC = "../linux";
my $fname = "$SRC/drivers/media/dvb/frontends/Makefile";

####################
# Get Makefile rules
#
sub get_makefile($)
{
	my $file = shift;
	my %rules;
	my %composite;

	open IN, $file or die "Can't find $file\n";
	while (<IN>) {
		# Handle line continuations
		if (/\\\n$/) {
		$_ .= <IN>;
		redo;
		}
		# Eat line continuations in string we will parse
		s/\s*\\\n\s*/ /g;

		if (m/(^\s*[[\da-zA-Z-_]+)-objs\s*[\:\+]*\=\s*(.*)\n/) {
			my $dep=$1;
			my $file = $2;
			$file =~ s/\.o / /g;
			$file =~ s/\.o$//;

			if ($file eq "") {
				die "broken dep on file $file for $dep\n";
			}

			$composite{$dep} = $file;
			printf "MULTI: $dep = $file\n" if ($debug > 1);
		}

		if (m/^\s*obj\-\$\(CONFIG_([^\)]+)\)\s*[\:\+]*\=\s*(.*)\n/) {
			my $rule = $1;
			my $file = $2;

			$file =~ s/\.o / /g;
			$file =~ s/\.o$//;

			$rules{$rule} = $file;
			printf "RULE: $rule = $file\n" if ($debug > 1);
		}
	}
	close IN;

	return (\%rules, \%composite);
}

###########################
# Seeks header dependencies
#
my %header_deps;

# For a more complete check, use:
# my $hfiles = "*.c";
my $hfiles = "av7110_av.c av7110.c av7110_ca.c av7110_hw.c av7110_ipack.c av7110_ir.c av7110_v4l.c budget-patch.c dvb_ringbuffer.c nova-t-usb2.c umt-010.c";

sub get_header_deps()
{
	my $file = shift;
	my %rules;
	my %composite;

	open IN, "gcc -I ../linux/include -I . -DCONFIG_PCI -D__LITTLE_ENDIAN -D_COMPAT_H -DKERNEL_VERSION\\(a,b,c\\) -MM $hfiles|";
	while (<IN>) {
		# Handle line continuations
		if (/\\\n$/) {
		$_ .= <IN>;
		redo;
		}
		# Eat line continuations in string we will parse
		s/\s*\\\n\s*/ /g;

		if (m/^([^\:]+)\s*\:\s*(.*)/) {
			my $dep = $1;
			my $file = $2;

			$dep =~ s|.*/||;
			$dep =~ s/\.o$//;

			my @files = split(/\s/, $file);
			foreach my $f (@files) {
				$f =~ s|.*/||;

				if (!defined($header_deps{$f})) {
					$header_deps{$f} = $dep;
				} else {
					$header_deps{$f} .=  " " . $dep;
				}

			}
		}
	}
	close IN;

	if ($debug > 1) {
		print "Header deps for: ";
		print "$_ " foreach %header_deps;
		print "\n";
	}
}


###########################
# Seeks files for Makefiles
#

my %driver_config;

sub parse_makefiles()
{
	my $fname = $File::Find::name;

	return if !($fname =~ m|/Makefile$|);
	return if ($fname =~ m|drivers/media/dvb/frontends/|);


	my ($refs, $mult) = get_makefile($fname);

	foreach my $ref (keys %$refs) {
		my $file=$$refs{$ref};

		my @files = split(/\s/, $file);
		foreach my $f (@files) {
			if (defined($$mult{$f})) {
				$file .= " " . $$mult{$f};
			}
		}

		$file =~ s|/||g;

		@files = split(/\s/, $file);
		foreach my $f (@files) {
			$driver_config{$f} = $ref;
		}
		if ($debug > 1) {
			print "$ref = ";
			print "$_ " foreach @files;
			print "\n";
		}
	}
}


########################
# Seeks files for header
#
my %select;

sub found_ref($$)
{
	my $file = shift;
	my $header = shift;
	my $found = 0;
	my $name = $file;
	$name =~ s|.*/||;

	$name =~ s/flexcop-fe-tuner.c/b2c2-flexcop/;
	$name =~ s/av7110.c/av7110.h/;

	if (defined ($header_deps{$name})) {
		$name = $header_deps{$name};
	} else {
		$name =~ s/\.[ch]$//;
	}

	my @files = split(/\s/, $name);
	foreach my $n (@files) {
		if (defined($driver_config{$n})) {
			my $ref = $driver_config{$n};
			printf "$ref needs %s\n", $header if ($debug);

			if ($ref =~ m/(PVRUSB2|CX23885|CX88|EM28XX|SAA3134)/) {
				$ref .="_DVB";
			}

			if (!defined($select{$ref})) {
				$select{$ref} = $header;
			} else {
				$select{$ref} .= " " . $header;
			}
			$found = 1;
		}
	}

	if (!$found) {
		printf "$file needs %s\n", $header;
	}
}

########################
# Seeks files for header
#

my %header;

sub parse_headers()
{
	my $file = $File::Find::name;

	return if !($file =~ m/\.[ch]$/);
	return if ($file =~ m|drivers/media/dvb/frontends/|);

	open IN, $file or die "Can't open $file\n";
	while (<IN>) {
		if (m/^\s*\#include\s+\"([^\"]+)\"/) {
			if (defined($header{$1})) {
				my $head = $header{$1};
				found_ref ($file, $head);
			}
		}
	}
	close IN;
}

########################
# Rewrite Kconfig's
#

sub parse_kconfigs()
{
	my $file = $File::Find::name;
	my $conf;
	my $out = "";
	my $tmp = "";
	my $all_sels;

	return if !($file =~ m/Kconfig$/);
	return if ($file =~ m|drivers/media/dvb/frontends/|);

	open IN, $file or die "Can't open $file\n";
	while (<IN>) {
		if (m/^config\s([A-Za-z_\-\d]+)/) {
			$out .= $tmp;
			if (defined($select{$1})) {
				$conf = $select{$1};
				$all_sels = " ". $conf. " ";
				$tmp = $_;

				printf "$file: rewriting headers for $1. It should select: %s\n", $all_sels if ($debug);
			} else {
				$conf = "";
				$out .= $_;
				$tmp = "";
			}
			next;
		}
		if (!$conf) {
			$out .= $_;
			next;
		}

		if (m/^\s*select\s+([A-Za-z_\-\d]+)/) {
			my  $op = $1;

			if (!$all_sels =~ m/\s($op)\s/) {
				# Drops line
				printf "$file: droppingg line $_\n";

				next;
			} else {
				$all_sels =~ s/\s($op)\s/ /;
			}
		}
		if (m/^[\s\-]*help/) {
			my @sel = split(/\s/, $all_sels);
			foreach my $s (@sel) {
				if ($s ne "") {
					printf "$file: Adding select for $s\n";
					$tmp .= "\tselect $s if !DVB_FE_CUSTOMISE\n";
				}
			}
		}
		$tmp .= $_;
	}
	close IN;

	$out .=$tmp;
	open OUT, ">$file" or die "Can't open $file\n";
	print OUT $out;
	close OUT;
}

#####
#main

get_header_deps();

my ($FEs, $mult) = get_makefile($fname);

foreach my $fe (keys %$FEs) {
	my $file=$$FEs{$fe};
	my $found = 0;

	# Special cases
	$file =~ s/tda10021/tda1002x/;
	$file =~ s/tda10023/tda1002x/;
	$file =~ s/dib3000mb/dib3000/;

	if (defined($$mult{$file})) {
		$file .= " ".$$mult{$file};
	}

	my @files = split(/\s/, $file);
	foreach my $f (@files) {
		if (stat("$f.h")) {
			printf "$fe = $f.h\n" if ($debug);
			$found = 1;
			$header {"$f.h"} = $fe;
			last;
		}
	}

	if (!$found) {
		printf "$file.h ($fe) not found in $file\n";
		exit -1;
	}
}

find({wanted => \&parse_makefiles, no_chdir => 1}, $SRC);
find({wanted => \&parse_headers, no_chdir => 1}, $SRC);
find({wanted => \&parse_kconfigs, no_chdir => 1}, $SRC);
