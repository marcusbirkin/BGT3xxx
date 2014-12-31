#!/usr/bin/perl

# The purpose of this script is to produce a file named '.myconfig', in
# the same style as the '.config' file.  Except .myconfig has disabled
# options explicitly set to 'n' rather than just omitted.  This is to
# make sure they override any corresponding options that may be turned on
# in the Kernel's config files.
# The .myconfig file is what will be included in the v4l-dvb Makefile
# to control which drivers are built.

my %config = ();
my %allconfig = ();

open IN,".config";
while (<IN>) {
	if (m/\s*(\w+)=\s*(\S*)/) {
#printf "%s=%s\n",$1,$2;
		$config { $1 } = $2;
	}
}
close IN;

# Build table of _all_ bool and tristate config variables
my $key = 0;
open IN,"Kconfig";
while (<IN>) {
	if (/^(?:menu)?config\s+(\w+)\s*$/) {
		$key == 0 or die "Couldn't find type of config '$key'";
		$key = "CONFIG_$1";
	} elsif (/^\s+bool(ean)?\s/) {
		$allconfig{$key} = 'bool';
		$key = 0;
	} elsif (/^\s+tristate\s/) {
		$allconfig{$key} = 'tristate';
		$key = 0;
	} elsif (/^\s+(int|hex|string)\s/) {
		$allconfig{$key} = 'data';
		$key = 0;
	}
}
close IN;

exists $allconfig{0} and die "Unable to correctly parse Kconfig file";

# Produce output for including in a Makefile
# Explicitly set bool/tri options that didn't appear in .config to n
# 'data' options are only output if they appeared in .config
open OUT,">.myconfig";
while ( my ($key, $value) = each(%allconfig) ) {
	if ($value eq 'data') {
		next unless (exists $config{$key});
		$value = $config{$key};
	} else {
		$value = exists $config{$key} ? $config{$key} : 'n';
	}
	printf OUT "%-44s := %s\n",$key,$value;
}
close OUT;
