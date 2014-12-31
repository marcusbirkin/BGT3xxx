#!/usr/bin/perl -w
use strict;

my $new_entry = -1;
my $nr = 0;
my ($id,$subvendor,$subdevice);
my %data;

my $H = shift;
my $C = shift;
my %blacklist;

open IN, "<$H";
while (<IN>) {
	# defines in header file
	if (/#define\s+(TUNER_\w+)\s+(\d+)/) {
		my $num=$2;
		$data{$1}->{nr} = $num;
		if (/#define\s+TUNER_TDA9887/) {
			$blacklist{$num}=1;
		}
		next;
	}
}
close IN;

open IN, "<$C";
while (<IN>) {
	# tuners
	if (/\[(TUNER_\w+)\]/) {
		$id = $1;
		if ($id =~ m/TUNER_MAX/) {
			next;
		}
		$data{$id}->{id} = $id;
	};
	next unless defined($id);

	if (!defined($data{$id}) || !defined($data{$id}->{name})) {
		$data{$id}->{name} = $1 if (/\.name\s*=\s*\"([^\"]+)\"/);
	}
}

foreach my $item (sort { $data{$a}->{nr} <=> $data{$b}->{nr} } keys %data) {
	if ($blacklist{$data{$item}->{nr}}) {
		next;
	}

	printf("tuner=%d - %s", $data{$item}->{nr}, $data{$item}->{name});
	print "\n";
}
