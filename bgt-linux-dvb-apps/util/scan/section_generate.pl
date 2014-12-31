#!/usr/bin/perl -w

use strict;

die "no section perl file given" unless @ARGV;

my $h = require($ARGV[0]);

our $basename;
our $debug = $ARGV[1];

($basename) = $ARGV[0] =~ /([a-zA-Z0-9_\-_]+).pl/;

local *H;
local *C;

h_header();
c_header();

foreach (sort keys %{$h}) {
	foreach my $item (@{$h->{$_}}) {
		if ($_ eq "descriptors") {
			printf H ("#define %s_ID 0x%02X\n",uc($item->{name}),$item->{id});
		}

		do_it  ($item->{name},$item->{elements});
	}
}

h_footer();
c_footer();

sub type
{
	if ($_[0] > 16) {
		return "u32";
	} elsif ($_[0] > 8) {
		return "u16";
	} else {
		return "u8 ";
	}
}

sub do_it
{
	my ($name,$val) = @_;
	print H "struct $name {\n";

	print C <<EOL;
struct $name read_$name(const u8 *b)
{
	struct $name v;
EOL
	my $offs = 0;
	for (my $i = 0; $i < scalar @{$val}; $i+=2) {
		printf H ("\t\t%s %-25s :%2d;\n",type($val->[$i+1]),$val->[$i],$val->[$i+1]);

		printf C ("\tv.%-25s = getBits(b,%3d,%2d);\n",$val->[$i],$offs,$val->[$i+1]);
		printf C ("\tfprintf(stderr,\"  %s = %%x %%d\\n\",v.%s,v.%s);\n",$val->[$i],$val->[$i],$val->[$i]) if $debug;
		$offs += $val->[$i+1];
	}
	print H "} PACKED;\n";
	print H "struct $name read_$name(const u8 *);\n\n";

	print C "\treturn v;\n}\n\n"
}

sub h_header
{
	open(H,">$basename.h");
	print H "#ifndef __".uc($basename)."_H_\n";
	print H "#define __".uc($basename)."_H_\n\n";
	print H "#include \"section.h\"\n\n";
}

sub c_header
{
	open(C,">$basename.c");
	print C "#include \"$basename.h\"\n\n";
}


sub c_footer
{
	close(C);
}

sub h_footer
{
	print H "#endif\n";
	close(H);
}
