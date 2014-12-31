#!/usr/bin/perl -w

# This script is used to generate hwdata pci info. We are going to 
# submit this data periodically to linux PCI ID's Project http://pciids.sf.net

my %map = (
	   "PCI_ANY_ID"            => "0",
	   "PCI_VENDOR_ID_PHILIPS" => "1131",
	   "PCI_VENDOR_ID_ASUSTEK" => "1043",
	   "PCI_VENDOR_ID_MATROX"  => "102B",
	   "PCI_VENDOR_ID_ATI"     => "1002",
);

sub fix_id($) {
	my $id = shift;
	$id = $map{$id} if defined($map{$id});
	$id =~ s/^0x//;
	return $id;
}

open ($input, "../saa7134-cards.c");

print ("\n");
print ("1131  Philips Semiconductors\n");

my %data;

while (<$input>) {

	if (/\[(SAA7134_BOARD_\w+)\]/) {
		$id = $1;
		$data{$id}->{id} = $id;
		$data{$id}->{subvendor} = "0";
	};
	next unless defined($id);
	
	if (!defined($data{$id}) || !defined($data{$id}->{name})) {
		$data{$id}->{name} = $1 if (/\.name\s*=\s*\"([^\"]+)\"/);
	}
	
	# saa7134_pci_tbl
	$device = $1 if (/\.device\s*=\s*(\w+),/);
	$subvendor = fix_id($1) if (/\.subvendor\s*=\s*(\w+),/);
	$subdevice = fix_id($1) if (/\.subdevice\s*=\s*(\w+),/);
	if (/.driver_data\s*=\s*(\w+),/) {
		if (defined($data{$1})  &&
		    defined($subvendor) && $subvendor ne "0" &&
		    defined($subdevice) && $subdevice ne "0" &&
		    defined($device)) {
			$data{$1}->{device} = $device;
			$data{$1}->{subvendor} = $subvendor;
			$data{$1}->{subdevice} = $subdevice;
			undef $device;
			undef $subvendor;
			undef $subdevice;
		}
	}
}

sub print_cards ($) {
    my $filter = shift;
    foreach my $item (sort {$data{$a}->{subvendor} cmp $data{$b}->{subvendor}} keys (%data)) 
    {
	if (defined ($data{$item}->{device}) && ($data{$item}->{device} eq $filter)) {
	    	printf("\t\t");
		printf("%s %s  %s", $data{$item}->{subvendor}, $data{$item}->{subdevice}, $data{$item}->{name});
		print "\n";
	}
    }
}

print ("\t7130  SAA7130 Video Broadcast Decoder\n");
print_cards ("PCI_DEVICE_ID_PHILIPS_SAA7130");
print ("\t7133  SAA7133/SAA7135 Video Broadcast Decoder\n");
print_cards ("PCI_DEVICE_ID_PHILIPS_SAA7133");
print ("\t7134  SAA7134 Video Broadcast Decoder\n");
print_cards ("PCI_DEVICE_ID_PHILIPS_SAA7134");

open ($input, "../cx88-cards.c");

%data=();

while (<$input>) {
	if (/\[(CX88_BOARD_\w+)\]/) {
		$id = $1;
		$data{$id}->{id} = $id;
		$data{$id}->{subvendor} = "0";
	};
	next unless defined($id);
	
	if (!defined($data{$id}) || !defined($data{$id}->{name})) {
		$data{$id}->{name} = $1 if (/\.name\s*=\s*\"([^\"]+)\"/);
	}

	$subvendor = fix_id($1) if (/\.subvendor\s*=\s*(\w+),/);
	$subdevice = fix_id($1) if (/\.subdevice\s*=\s*(\w+),/);
	if (/.card\s*=\s*(\w+),/) {
		if (defined($data{$1})  &&
		    defined($subvendor) && $subvendor ne "0" &&
		    defined($subdevice) && $subdevice ne "0") {
			$data{$1}->{subvendor} = $subvendor;
			$data{$1}->{subdevice} = $subdevice;
			undef $subvendor;
			undef $subdevice;
		}
	}
}

print ("14f1  Conexant\n");
print ("\t8800  CX23880/1/2/3 PCI Video and Audio Decoder\n");

foreach my $item (sort {$data{$a}->{subvendor} cmp $data{$b}->{subvendor}} keys (%data)) 
{
	if (defined ($data{$item}->{subdevice}) && ($data{$item}->{subvendor} ne "0")) {
		printf("\t\t");
    		printf("%s %s  %s", $data{$item}->{subvendor}, $data{$item}->{subdevice}, $data{$item}->{name});
		print "\n";
	}
}

print ("109e  Brooktree Corporation\n");
print ("\t032e  Bt878 Video Capture\n");
%data=();

open ($input, "../bttv-cards.c");
while (<$input>) {
    if (/\{\s*0x(\w{4})(\w{4}),.*\"([^\"]+)\"\s\},/)
	{
		printf ("\t\t%s %s  %s\n", $2, $1, $3);
	}
}

printf ("\n");
