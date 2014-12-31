#!/usr/bin/perl -w
use strict;

my $new_entry = -1;
my $nr = 0;
my ($id,$subvendor,$subdevice);
my %data;

while (<>) {
	# defines in header file
	if (/#define\s*(\w+)\s*(\d+)/) {
		$data{$1}->{nr} = $2;
		next;
	}
	# boards
	if (/^\s*\[([\w\d_]+)\]\s*=\s*{/) {
		$id = $1;
		$data{$id}->{id} = $id;
	};

	next unless defined($id);

	if (/USB_DEVICE.*0x([0-9a-fA-F]*).*0x([0-9a-fA-F]*).*/)
	{
		$subvendor=$1;
		$subdevice=$2;
	}
	if(/driver_info\s*=\s*([\w\d_]+)/)
	{
		push @{$data{$1}->{subid}}, "$subvendor:$subdevice";
	}
	if (!defined($data{$id}) || !defined($data{$id}->{name})) {
		$data{$id}->{name} = $1 if (/\.ModelString\s*=\s*\"([^\"]+)\"/);
	}
}

foreach my $item (sort { $data{$a}->{nr} <=> $data{$b}->{nr} } keys %data) {
	printf("%3d -> %-56s", $data{$item}->{nr}, $data{$item}->{name});
	printf(" [%s]",join(",",@{$data{$item}->{subid}}))
	  if defined($data{$item}->{subid});
	print "\n";
}
