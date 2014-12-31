#!/usr/bin/perl -w
use strict;

my $new_entry = -1;
my $nr = 0;
my ($id,$subvendor,$subdevice);
my %data;
my $pciid=0;

while (<>) {
	# defines in header file
        if (/#define\s+(BTTV_BOARD_\w+)\s+0x([0-9a-fA-F]*).*/) {
		$data{$1}->{nr} = hex $2;

		next;
	}
	# cx88_boards
	if (/\[(BTTV_BOARD_\w+)\]/) {
		$id = $1;
		$data{$id}->{id} = $id;
#		$data{$id}->{nr} = $nr++;
	};
	if (/0x([0-9a-fA-F]...)([0-9a-fA-F]...)/) {
		$subvendor = $2;
		$subdevice = $1;
		if (/(BTTV_BOARD_\w+)/) {
			push @{$data{$1}->{subid}}, "$subvendor:$subdevice";
			undef $subvendor;
			undef $subdevice;
		}
	}

	next unless defined($id);

	if (!defined($data{$id}) || !defined($data{$id}->{name})) {
		$data{$id}->{name} = $1 if (/\.name\s*=\s*\"([^\"]+)\"/);
	}

	if (/0x([0-9]...)([0-9]...)/) {
		$subvendor = $1;
		$subdevice = $2;
	}

}

foreach my $item (sort { $data{$a}->{nr} <=> $data{$b}->{nr} } keys %data) {
	printf("%3d -> %-51s", $data{$item}->{nr}, $data{$item}->{name});
	printf(" [%s]",join(",",@{$data{$item}->{subid}}))
	  if defined($data{$item}->{subid});
	print "\n";
}
