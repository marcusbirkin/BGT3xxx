#!/usr/bin/perl -w
use strict;

my %map = (
	   "PCI_ANY_ID"            => "0",
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
	if (/#define\s+(CX23885_BOARD_\w+)\s+(\d+)/) {
		$data{$1}->{nr} = $2;
		next;
	}
	# cx88_boards
	if (/\[(CX23885_BOARD_\w+)\]/) {
		$id = $1;
		$data{$id}->{id} = $id;
#		$data{$id}->{nr} = $nr++;
	};
	next unless defined($id);

	if (!defined($data{$id}) || !defined($data{$id}->{name})) {
		$data{$id}->{name} = $1 if (/\.name\s*=\s*\"([^\"]+)\"/);
	}

	# cx88_pci_tbl
	$subvendor = fix_id($1) if (/\.subvendor\s*=\s*(\w+),/);
	$subdevice = fix_id($1) if (/\.subdevice\s*=\s*(\w+),/);
	if (/.card\s*=\s*(\w+),/) {
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
	printf("%3d -> %-51s", $data{$item}->{nr}, $data{$item}->{name});
	printf(" [%s]",join(",",@{$data{$item}->{subid}}))
	  if defined($data{$item}->{subid});
	print "\n";
}
