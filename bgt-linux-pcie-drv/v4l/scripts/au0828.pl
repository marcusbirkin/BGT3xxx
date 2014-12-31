#!/usr/bin/perl -w
use strict;

my $new_entry = -1;
my $nr = 0;
my ($id,$subvendor,$subdevice);
my %data;

while (<>) {
	# defines in header file
	if (/#define\s+(AU08[\d]._BOARD_\w+)\s+(\d+)/) {
		$data{$1}->{nr} = $2;
		next;
	}
	# au0828_boards
	if (/\[(AU0828_BOARD_\w+)\]/) {
		$id = $1;
		$data{$id}->{id} = $id;
		$data{$id}->{type} = "(au0828)";
#		$data{$id}->{nr} = $nr++;
	};

	next unless defined($id);

	if (/USB_DEVICE.*0x([0-9a-fA-F]*).*0x([0-9a-fA-F]*)/ ) {
		$subvendor=$1;
		$subdevice=$2;
	}

	if (/.*driver_info.*(AU08[\d]._BOARD_\w+)/ ) {
		push @{$data{$1}->{subid}}, "$subvendor:$subdevice";
	}

	if (!defined($data{$id}) || !defined($data{$id}->{name})) {
		$data{$id}->{name} = $1 if (/\.name\s*=\s*\"([^\"]+)\"/);
	}

	# au0828_USB_tbl


}

foreach my $item (sort { $data{$a}->{nr} <=> $data{$b}->{nr} } keys %data) {
	printf("%3d -> %-40s %-15s", $data{$item}->{nr}, $data{$item}->{name},$data{$item}->{type});
	printf(" [%s]",join(",",@{$data{$item}->{subid}}))
	  if defined($data{$item}->{subid});
	print "\n";
}
