#!/usr/bin/perl
# Copyright (C) 2006 Trent Piepho <xyzzy@speakeasy.org>
#
# Look for lines in C files like "#ifdef CONFIG_SOME_KCONF_VAR" and make
# sure CONFIG_SOME_KCONF_VAR is something that exists.

use strict;

if($#ARGV < 0) {
	print "Usage: $0 kernel_dir [files to check ...]\n\n";
	print "If no files are listed, checks all files from hg manifest\n";
	exit;
}
my $kdir = shift;

if($#ARGV < 0) {
	@ARGV = `hg manifest | cut "-d " -f3 | grep \\.[ch]\$`;
	$? != 0 and die "Error getting manifest: $!";
	chomp @ARGV;
}

my %kconfigs;		# List of Kconfig files read in already
my %vars;		# List of defined variables
sub readkconfig($$)
{
	my $fn = "$_[0]/$_[1]";
	# Don't read the same kconfig file more than once.  This also means
	# the drivers/media/Kconfig file from kernel won't be read in addition
	# to the one from v4l-dvb.
	return if exists $kconfigs{$_[1]};
	$kconfigs{$_[1]} = 1;
#	print "Reading $fn\n";
	my $fh;
	open $fh, '<', "$fn" or die "Can't read Kconfig file $fn: $!";
	while(<$fh>) {
		if (/^\s*source\s+"([^"]+)"\s*$/ || /^\s*source\s+(\S+)\s*$/) {
			readkconfig($_[0], $1);
		} elsif(/^(?:menu)?config\s+(\w+)$/) {
			$vars{"CONFIG_$1"}=1;
		}
	}
	close $fh;
}

readkconfig('linux', 'drivers/media/Kconfig');
foreach(glob "$kdir/arch/*/Kconfig") {
	s|^\Q$kdir/\E||;
	next if(m|arch/um/Kconfig|);
	readkconfig($kdir, $_);
}

while(<>) {
	if(/^\s*#ifn?def\s+(CONFIG_\w+?)(:?_MODULE)?\W*$/) {
#		print "Found $1\n";
		print "Unknown config $1 in $ARGV:$.\n" unless(exists $vars{$1});
		next;
	}
	if(/^\s*#if/) {
		$_ .= <> while(/\\$/);	# Handle line continuations
		my $last;
		while(/defined\(\s*(CONFIG_\w+?)(_MODULE)?\s*\)/) {
			$_ = $';
			next if($last eq $1);
			$last = $1;
			print "Unknown config $1 in $ARGV:$.\n" unless(exists $vars{$1});
		}
	}
} continue {
	close ARGV if eof;
}
