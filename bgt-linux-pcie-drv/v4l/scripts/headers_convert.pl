#!/usr/bin/perl
#
# headers_install prepare the listed header files for use in
# user space and copy the files to their destination.
#

use strict;
use warnings;

foreach (@ARGV) {
	my $file = $_;
	my $tmpfile = $file . ".tmp";

	open(my $infile, '<', "$file")
		or die "$file: $!\n";
	open(my $outfile, '>', "$tmpfile") or die "$tmpfile: $!\n";
	while (my $line = <$infile>) {
		$line =~ s/([\s(])__user\s/$1/g;
		$line =~ s/([\s(])__force\s/$1/g;
		$line =~ s/([\s(])__iomem\s/$1/g;
		$line =~ s/\s__attribute_const__\s/ /g;
		$line =~ s/\s__attribute_const__$//g;
		$line =~ s/^#include <linux\/compiler.h>//;
		printf $outfile "%s", $line;
	}
	close $outfile;
	close $infile;
	system "mv $tmpfile $file";
}
exit 0;
