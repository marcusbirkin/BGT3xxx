#!/usr/bin/perl -w
use strict;

my %map = (
	   "PCI_ANY_ID"            => "0",
	   "PCI_VENDOR_ID_PHILIPS" => "1131",
	   "PCI_VENDOR_ID_ASUSTEK" => "1043",
	   "PCI_VENDOR_ID_MATROX"  => "102B",
);

sub fix_id($) {
	my $id = shift;
	$id = $map{$id} if defined($map{$id});
	$id =~ s/^0x//;
	return $id;
}

my $new_entry = -1;
my $nr = 0;
my ($id,$subvendor,$subdevice);
my %data;

while (<>) {
	# defines in header file
	if (/#define\s+(SAA7134_BOARD_\w+)\s+(\d+)/) {
		$data{$1}->{nr} = $2;
		next;
	}
	# saa7134_boards
	if (/\[(SAA7134_BOARD_\w+)\]/) {
		$id = $1;
		$data{$id}->{id} = $id;
#		$data{$id}->{nr} = $nr++;
	};
	next unless defined($id);

	if (!defined($data{$id}) || !defined($data{$id}->{name})) {
		$data{$id}->{name} = $1 if (/\.name\s*=\s*\"([^\"]+)\"/);
	}

	# saa7134_pci_tbl
	$subvendor = fix_id($1) if (/\.subvendor\s*=\s*(\w+)\s*,*/);
	$subdevice = fix_id($1) if (/\.subdevice\s*=\s*(\w+)\s*,*/);
	if (/.driver_data\s*=\s*(\w+)\s*,*/) {
		if (defined($data{$1})  &&
		    defined($subvendor) && $subvendor ne "0" &&
		    defined($subdevice) && $subdevice ne "0") {
			push @{$data{$1}->{subid}}, "$subvendor:$subdevice";
			undef $subvendor;
			undef $subdevice;
		}
	}
}

foreach my $item (sort { $data{$a}->{nr} <=> $data{$b}->{nr} } keys %data) {
	printf("%3d -> %-40s", $data{$item}->{nr}, $data{$item}->{name});
	printf(" [%s]",join(",",@{$data{$item}->{subid}}))
	  if defined($data{$item}->{subid});
	print "\n";
}
