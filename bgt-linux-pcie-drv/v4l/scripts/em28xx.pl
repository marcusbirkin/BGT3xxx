#!/usr/bin/perl -w
use strict;

my $new_entry = -1;
my $nr = 0;
my ($id,$subvendor,$subdevice);
my %data;

my $debug = 0;

while (<>) {
	# defines in header file
	if (/#define\s+(EM2[\d][\d][\d]_BOARD_[\w\d_]+)\s+(\d+)/) {
		printf("$1 = $2\n") if ($debug);
		$data{$1}->{nr} = $2;
		next;
	}
	# em2820_boards
	if (/\[(EM2820_BOARD_[\w\d_]+)\]/) {
		$id = $1;
		printf("ID = $id\n") if $debug;
		$data{$id}->{id} = $id;
		$data{$id}->{type} = "(em2820/em2840)";
#		$data{$id}->{nr} = $nr++;
	} elsif (/\[(EM)(2[\d]..)(_BOARD_[\w\d_]+)\]/) {
		$id = "$1$2$3";
		printf("ID = $id\n") if $debug;
		$data{$id}->{id} = $id;
		$data{$id}->{type} = "(em$2)";
#		$data{$id}->{nr} = $nr++;
	};

	next unless defined($id);

	if (/USB_DEVICE.*0x([0-9a-fA-F]*).*0x([0-9a-fA-F]*)/ ) {
		$subvendor=$1;
		$subdevice=$2;
	}

	if (/.*driver_info.*(EM2[\d].._BOARD_[\w\d_]+)/ ) {
		push @{$data{$1}->{subid}}, "$subvendor:$subdevice";
	}

	if (!defined($data{$id}) || !defined($data{$id}->{name})) {
		$data{$id}->{name} = $1 if (/\.name\s*=\s*\"([^\"]+)\"/);
		if (defined $data{$id}->{name} && $debug) {
			printf("name[$id] = %s\n", $data{$id}->{name});
		}
	}

	# em2820_USB_tbl


}

foreach my $item (sort { $data{$a}->{nr} <=> $data{$b}->{nr} } keys %data) {
	printf("%3d -> %-40s %-15s", $data{$item}->{nr}, $data{$item}->{name}, $data{$item}->{type});
	printf(" [%s]",join(",",@{$data{$item}->{subid}}))
	  if defined($data{$item}->{subid});
	print "\n";
}
