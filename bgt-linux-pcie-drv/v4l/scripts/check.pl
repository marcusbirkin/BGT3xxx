#!/usr/bin/perl
use Getopt::Std;
use strict;

my $c_syntax=0;
my $fmt="--emacs";

my %opt;

sub usage ($)
{
	my $name = shift;
	printf "Usage: %s [<args>] [<file>]\n".
		"\t-c\tc style\n".
		"\t-e\emacs style (default)\n".
		"\t-t\terse style (default)\n".
		"\t<file>\tfile name to open. If file not specified, uses hg diff\n\n", $name;

	exit -1;
}

if (not getopts('cet',\%opt) or defined $opt{'h'}) {
	usage($0);
}

my $cmd=shift;

if ($opt{'c'}) {
	$c_syntax=1;
}

if ($opt{'t'}) {
	$fmt="--terse";
	$c_syntax=0;
}

if ($cmd) {
	$cmd="diff -upr /dev/null $cmd";
} else {
	$cmd="hg diff";
}

my $checkpatch=$ENV{CHECKPATCH};

if (!$checkpatch) {
	$checkpatch="/lib/modules/`uname -r`/build/scripts/checkpatch.pl";
}

my $cp_version;
open IN,"$checkpatch|";
while (<IN>) {
	tr/A-Z/a-z/;
	if (m/version\s*:\s*([\d\.]+)/) {
		$cp_version = $1;
	}
}
close IN;

my $intree_checkpatch = "scripts/checkpatch.pl --no-tree";
if (!open IN,"$intree_checkpatch|") {
	$intree_checkpatch = "v4l/".$intree_checkpatch;
	open IN,"$intree_checkpatch|";
}
while (<IN>) {
	tr/A-Z/a-z/;
	if (m/version\s*:\s*([\d\.]+)/) {
		if ($1 > $cp_version) {
			print "# WARNING: $checkpatch version $cp_version is\n"
			      ."#         older than $intree_checkpatch version"
			      ." $1.\n#          Using in-tree one.\n#\n";
			$cp_version = $1;
			$checkpatch = $intree_checkpatch;
		}
	}
}
close IN;

open IN,"$cmd | $checkpatch -q --nosignoff $fmt -|";

my $err="";
my $errline="";
my $file="";
my $ln_numb;

my $pwd=`pwd`;
$pwd =~ s|/[^/]+\n$||;

sub print_err()
{
	if ($err =~ m/LINUX_VERSION_CODE/) {
		return;
	}

	if ($err) {
		printf STDERR "%s/%s: In '%s':\n", $pwd, $file, $errline;
		printf STDERR "%s/%s:%d: %s\n", $pwd, $file, $ln_numb, $err;
		$err="";
	}
}

if ($c_syntax == 0) {
	while (<IN>) {
		s|^#[\d]+:\s*FILE:\s*|../|;
		print "$_";
	}
} else {
	while (<IN>) {
		if (m/^\+(.*)\n/) {
			$errline=$1;
		} elsif (m/^\#\s*[\d]+\s*:\s*FILE:\s*([^\:]+)\:([\d]+)/) {
			$file=$1;
			$ln_numb=$2;
		} elsif (m/^\-\s*\:\d+\:\s*(.*)\n/) {
			print_err();
			$err = $1;
			$err =~ s/WARNING/warning/;
		}
#		print "# $_";
	}
}
close IN;
print_err();
