#!/usr/bin/perl

# Copyright (C) 2006 Trent Piepho <xyzzy@speakeasy.org>
# Scans a tree of Linux Kernel style Makefiles, and builds lists of what
# builds what.
# Outputs three lists:
# 1.  Kconfig variables associated with the kernel module(s) they turn on
# 2.  Kernel modules associated with their source file(s)
# 3.  Kconfig variables associated with all the source file(s) they turn on
#
# Optional options:
#   Prefix relative to source tree root to start scanning in.  This
#   will be stripped from the beginnings of all filenames printed out.
#     Default is 'linux/drivers/media'
#   Root of source tree
#     Default is to use 'hg root' command and if that fails the cwd
#
# Most usefull with grep, for example
# List of modules and source files used by DVB_BUDGET_AV
# deps.pl | grep DVB_BUDGET_AV
#
# Kconfig variable and kernel module that use dvb-usb-init.c
# deps.pl | grep dvb-usb-init.c
#
# Kconfig variable and source files that make dvb-core.ko
# deps.pl | grep dvb-core.ko
#
# Also has some ability to check Makefiles for errors
use strict;
use FileHandle;

# Print out some extra checks of Makefile correctness
my $check = 0;

# Directory to start in.  Will be stripped from all filenames printed out.
my $prefix = 'linux/drivers/media/';

# Root of source tree
my $root;

# List of Makefile's opened
my %makefiles = ();

# For each Kconfig variable, a list of modules it builds
my %config = ();

# For each module that is made up of multiple source files, list of sources
my %multi = ();

sub open_makefile($) {
    my $file = shift;

    # only open a given Makefile once
    return if exists $makefiles{$file};
    $makefiles{$file} = 1;

    $file =~ m|^(.*)/[^/]*$|;
    my $dir = $1;

#    print STDERR "opening $root$file (dir=$dir)\n";
    my $in = new FileHandle;
    open $in, '<', "$root$file" or die "Unable to open Makefile '$root$file': $!";

    while (<$in>) {
	# print STDERR "Line: $_";
	# Skip comment and blank lines
	next if (/^\s*(#.*)?$/);
	m/^\s*\-?include/ and die "Can't handle includes! In $file";

	# Handle line continuations
	if (/\\\n$/) {
	    $_ .= <$in>;
	    redo;
	}
	# Eat line continuations in string we will parse
	s/\s*\\\n\s*/ /g;
	# Eat comments
	s/#.*$//;

	if (/^\s*obj-(\S+)\s*([:+]?)=\s*(\S.*?)\s*$/) {
	    print STDERR "Should use '+=' in $file:$.\n$_\n" if ($check && $2 ne '+');
	    my ($var,$targets) = ($1, $3);
	    if ($var =~ /\$\(CONFIG_(\S+)\)$/) {
		$var = $1;
	    } elsif ($var !~ /^[ym]$/) {
		print STDERR "Confused by obj assignment '$var' in $file:$.\n$_";
	    }
	    foreach(split(/\s+/, $targets)) {
		if (m|/$|) { # Ends in /, means it's a directory
		    open_makefile("$dir/$_".'Makefile');
		} elsif (/^(\S+)\.o$/) {
		    push @{$config{$var}}, "$dir/$1";
		} else {
		    print STDERR "Confused by target '$_' in $file:$.\n";
		}
	    }
	    next;
	}
	if (/(\S+)-objs\s*([:+]?)=\s*(\S.*?)\s*$/) {
	    my @files = split(/\s+/, $3);
	    map { s|^(.*)\.o$|$dir/\1| } @files;
	    if ($2 eq '+') {
		# Adding to files
		print STDERR "Should use ':=' in $file:$.\n$_\n" if ($check && !exists $multi{"$dir/$1"});
		push @files, @{$multi{"$dir/$1"}};
	    } else {
		print STDERR "Setting objects twice in $file:$.\n$_\n" if ($check && exists $multi{"$dir/$1"});
	    }
	    $multi{"$dir/$1"} = \@files;
	    next;
	}
	if (/^\s*EXTRA_CFLAGS\s*([:+]?)=\s*(\S.*?)\s*$/) {
	    if ($check) {
		sub allI { /^-I/ or return 0 foreach split(/\s+/, $_[0]);return 1; }
		my $use = allI($2) ? ':' : '+';
		print STDERR "Should use '$use=' with EXTRA_CFLAGS in $file:$.\n$_\n"
		    if ($1 ne $use);
	    }
	    next;
	}
	print STDERR "Odd line $file:$.\n$_\n" if ($check);
    }
}

if ($#ARGV >= 0) {
    $prefix = $ARGV[0];
    $prefix .= '/' unless ($prefix =~ m|/$|);
}

# Find root of source tree: command line, then Hg command, lastly cwd
if ($#ARGV >= 1) {
    $root = $ARGV[1];
    $root .= '/' unless ($root =~ m|/$|);
} else {
    $root = `hg root 2>/dev/null`;
    if($? == 0) {
	chomp $root;
	$root .= '/';
    } else {
	$root = '';
    }
}
print "# Using source tree root '$root'\n" if ($root ne '');

open_makefile($prefix."Makefile");

print "# Kconfig variable = kernel modules built\n";
foreach my $var (keys %config) {
    my @list = @{$config{$var}};
    map { s/^$prefix(.*)$/\1.ko/ } @list;
    printf "%-22s= %s\n", $var, join(' ', @list);
}

print "\n# kernel module = source files\n";
my %modules = ();
foreach my $mods (values %config) {
    $modules{$_} = 1 foreach @$mods;
}
foreach (keys %modules) {
    /$prefix(.*)$/;
    printf "%-30s = ", "$1.ko";
    if (exists $multi{$_}) {
	my @list =  @{$multi{$_}};
	map { s/^$prefix(.*)$/\1.c/ } @list;
	print join(' ', @list), "\n";
    } else {
	print "$1.c\n";
    }
}

print "\n# Kconfig varible = source files\n";
while (my ($var, $list) = each %config) {
    my @outlist = ();
    foreach (@$list) {
	if (exists $multi{$_}) {
	    push @outlist, @{$multi{$_}};
	} else {
	    push @outlist, $_;
	}
    }
    map { s/^$prefix(.*)$/\1.c/ } @outlist;
    printf "%-22s= %s\n", $var, join(' ', @outlist);
}

exit;
