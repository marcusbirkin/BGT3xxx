#!/usr/bin/perl

# Copyright (C) 2008 Mauro Carvalho Chehab <mchehab@infradead.org>


use strict;
use File::Find;
use Fcntl ':mode';
use FileHandle;

my $debug=0;

my $SRC = 'linux';

my %export;

#############
# open_makefile were adapted from analyze_build.pl
#   by Copyright (C) 2006 Trent Piepho <xyzzy@speakeasy.org>

# Print out some extra checks of Makefile correctness
my $check = 0;

# Root of source tree
my $root;

# List of Makefile's opened
my %makefiles = ();

# For each module that is made up of multiple source files, list of sources
my %multi = ();
my $multi_count = 0;

my %config;

my %associate;

sub open_makefile($) {
	my $file = shift;

	# only open a given Makefile once
	return if exists $makefiles{$file};
	$makefiles{$file} = 1;

	$file =~ m|^(.*)/[^/]*$|;
	my $dir = $1;

	print "opening $root$file (dir=$dir)\n" if ($debug > 2);
	my $in = new FileHandle;
	open $in, '<', "$root$file" or die "Unable to open Makefile '$root$file': $!";

	while (<$in>) {
		# print STDERR "Line: $_";
		# Skip comment and blank lines
		next if (/^\s*(#.*)?$/);
		m/^\s*\-?include/ and die "Can't handle includes! In $file";

		# Handle line continuations
		if (/\\\n$/) {
			$_ .= <$in>;
			redo;
		}
		# Eat line continuations in string we will parse
		s/\s*\\\n\s*/ /g;
		# Eat comments
		s/#.*$//;

		if (/^\s*obj-(\S+)\s*([:+]?)=\s*(\S.*?)\s*$/) {
			print STDERR "Should use '+=' in $file:$.\n$_\n" if ($check && $2 ne '+');
			my ($var,$targets) = ($1, $3);
			if ($var =~ /\$\(CONFIG_(\S+)\)$/) {
				$var = $1;
			} elsif ($var !~ /^[ym]$/) {
				print STDERR "Confused by obj assignment '$var' in $file:$.\n$_";
			}
			foreach(split(/\s+/, $targets)) {
				if (m|/$|) { # Ends in /, means it's a directory
					open_makefile("$dir/$_".'Makefile');
				} elsif (/^(\S+)\.o$/) {
					$config{"$dir/$1"} = $var;
#					printf "%s -> %s\n", $var, $1 if $debug > 1;
				} else {
					print STDERR "Confused by target '$_' in $file:$.\n";
				}
			}
			next;
		}
		if (/(\S+)-objs\s*([:+]?)=\s*(\S.*?)\s*$/) {
			my @files = split(/\s+/, $3);
			map { s|^(.*)\.o$|$dir/\1| } @files;
			if ($2 eq '+') {
				# Adding to files
				print STDERR "Should use ':=' in $file:$.\n$_\n" if ($check && !exists $multi{"$dir/$1"});
				push @files, @{$multi{"$dir/$1"}};
			} else {
				print STDERR "Setting objects twice in $file:$.\n$_\n" if ($check && exists $multi{"$dir/$1"});
			}
			$multi{"$dir/$1"} = \@files;
			next;
		}
		if (/^\s*EXTRA_CFLAGS\s*([:+]?)=\s*(\S.*?)\s*$/) {
			if ($check) {
				sub allI { /^-I/ or return 0 foreach split(/\s+/, $_[0]);return 1; }
				my $use = allI($2) ? ':' : '+';
				print STDERR "Should use '$use=' with EXTRA_CFLAGS in $file:$.\n$_\n"
				if ($1 ne $use);
			}
			next;
		}
		print STDERR "Odd line $file:$.\n$_\n" if ($check);
	}
	close IN;
}

#
#############

sub associate_multi()
{
	foreach (keys %multi) {
		my $name = $_;
		my @files =  @{$multi{$_}};
		map { s/^(.*)$/\1.c/ } @files;

		foreach (@files) {
			my $file = $_;
			my $var = $config{$name};
			$associate{$file} = $var;
			printf "$var -> $file\n" if $debug > 1;
		}
		delete $config{$name};
	}
	foreach my $file (keys %config) {
		my $var = $config{$file};
		$file .= ".c";
		$associate{$file} = $var;
		printf "$var -> $file\n" if $debug > 1;
	}
}

sub build_exported_symbol_list {
	my $file = $File::Find::name;

	return if (!($file =~ /\.c$/));

	open IN, $file;
	while (<IN>) {
		if (m/EXPORT_SYMBOL.*\(\s*([^\s\)]+)/) {
			$export{$1} = $file;
			printf "%s -> %s\n", $file , $1 if $debug > 1;
		}
	}
	close IN;
}


sub find_usage_list {
	my %depend;
	my $file = $File::Find::name;
	my $s;

	return if (!($file =~ /\.c$/));

	open IN, $file;
	printf "Checking symbols at $file\n" if $debug;
	while (<IN>) {
		foreach my $symbol (keys %export) {
			my $symb_file = $export{$symbol};

			# Doesn't search the symbol at the file that defines it
			next if ($symb_file eq $file);

			if (m/($symbol)/) {
				my $var = $associate{$symb_file};
				if (!$depend{$var}) {
					printf "$symbol found at $file. It depends on %s\n", $associate{$symb_file} if $debug;
					$depend{$var} = 1;
				}
			}
		}
	}
	close IN;

	foreach (%depend) { $s .= "$_ && "; };
	$s =~ s/\&\&\ $//;
	if ($s ne "") {
		print $associate{$file}." depends on $s\n";
	}
}

print <<EOL;
Dependency check tool for Kernel Symbols.

Copyright(c) 2008 by Mauro Carvalho Chehab <mchehab\@infradead.org>
This code is licenced under the terms of GPLv2.

This script seeks all .c files under linux/ for their exported symbols. For
each exported symbol, it will check what Kconfig symbol is associated. Then, it
will cross-check that symbol usage and output a Kconfig depencency table.

WARNING: The result of this tool should be used just as a hint, since, due to
performance issues, and to simplify the tool, the checks will use a simple grep
for the symbol string at the .c files, instead of a real symbol cross-check.

Also, the same symbol may appear twice with different dependencies. This is due
to the way it checks for symbols. The final dependency is the union (AND) of
all showed ones for that symbol.

Further patches improving this tool are welcome.

EOL

print "Checking makefile rules..." if $debug;
open_makefile("$SRC/drivers/media/Makefile");
print " ok\n" if $debug;

print "Associating symbols with c files..." if $debug;
associate_multi();
print " ok\n" if $debug;

print "finding exported symbols at $SRC..." if $debug;
find({wanted => \&build_exported_symbol_list, no_chdir => 1}, $SRC);
print " ok\n" if $debug;

print "finding usage of symbols at $SRC\n" if $debug;
find({wanted => \&find_usage_list, no_chdir => 1}, $SRC);
print "finished\n" if $debug;

