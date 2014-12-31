#!/usr/bin/perl
use strict;

my $need_changes = 0;
my $out;

open IN, '<.config';
while (<IN>) {
	s/CONFIG_VIDEO_CX88_MPEG=y/CONFIG_VIDEO_CX88_MPEG=m/ and $need_changes=1;
	$out .= $_;
}
close IN;

if ($need_changes) {
	printf("There's a known bug with the building system with this kernel. Working around.\n");
	open OUT, '>.config';
	print OUT $out;
	close OUT;
}
