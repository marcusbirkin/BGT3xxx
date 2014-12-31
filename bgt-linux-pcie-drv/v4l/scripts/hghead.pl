#!/usr/bin/perl
use strict;

#################################################################
# analyse diffs

my $in = shift;
my $line;
my $subject;
my $sub_ok=0;
my $init=0;
my $num=0;
my $hgimport=0;
my $mmimport=0;
my $maint_ok=0;
my $noblank=1;
my $maintainer_name=$ENV{CHANGE_LOG_NAME};
my $maintainer_email=$ENV{CHANGE_LOG_EMAIL_ADDRESS};
my $from="";
my $body="";
my $priority="";
my $signed="";
my $fromname="";

open IN,  "<$in";

while ($line = <IN>) {
	if ($line =~ m/^\s*Index.*/) {
		last;
	}
	if ($line =~ m/^diff .*/) {
		last;
	}
	if ($line =~ m/^\-\-\- .*/) {
		last;
	}
	if ($line =~ m/^\-\-\-\-.*/) {
		$body="";
		next;
	}
	if ($line =~ m/^\-\-\-.*/) {
		last;
	}
	if ($line =~ m/^\+\+\+ .*/) {
		last;
	}

	if ($line =~ m/^#\s*Date\s*(.*)/) {
		print "#Date: $1\n";
		next;
	}

	if ($line =~ m/^Date:\s*(.*)/) {
		print "#Date: $1\n";
		next;
	}

	$line =~ s/^#\sUser/From:/;

	my $tag=$line;
	my $arg=$line;
	$tag =~ s/\s*([^\s]+:)\s*(.*)\n/\1/;
	$arg =~ s/\s*([^\s]+:)\s*(.*)\n/\2/;

	$tag =~ tr/A-Z/a-z/;

	if ($tag =~ m/^from:/) {
		if ($arg =~ m/^[\s\"]*([^\"]*)[\s\"]*<(.*)>/) {
			if ($1 eq "") {
				next;
			}
			my $name=$1;
			my $email=$2;
			$name =~ s/\s+$//;
			$email =~ s/\s+$//;
			$fromname="$name <$email>";
			$from= "From: $fromname\n";
			next;
		}
		if ($line =~ m/^From:\sakpm\@osdl.org/) {
			$mmimport=1;
			next;
		}
		print "Bad: author line have a wrong syntax: $line\n";
		die;
	}

	if ($tag =~ m/^subject:/) {
		$subject = "$arg\n";
		$sub_ok = 1;
		next;
	}

	if ($tag =~ m/^priority:/) {
		$arg =~ tr/A-Z/a-z/;

		# Replace the -git branch names for high/normal/low
		$arg =~ s/^fixes$/high/;
		$arg =~ s/^fix$/high/;
		$arg =~ s/^working$/normal/;
		$arg =~ s/^work$/normal/;
		$arg =~ s/^pending$/low/;
		$priority = "Priority: $arg";
		next;
	}

	if ($line =~ m;^ .*\/.*\| *[0-9]*;) {
		next;
	}
	if ($line =~m/\d+\s*file.* changed, /) {
		next;
	}

	if ($tag =~ m/^signed-off-by:.*/) {
		$noblank=1;
		if ($line =~ m/$maintainer_name/) {
			$maint_ok=1;
		}

		$signed="$signed$line";
		next;
	}
	if ( ($line =~ m/^\# HG changeset patch/) ||
	     ($line =~ m/^has been added to the -mm tree.  Its filename is/) ) {
		$sub_ok=0;
		$init=0;
		$num=0;
		$maint_ok=0;
		$noblank=1;
		$from="";
		$body="";
		$subject="";
		$hgimport=1;
		next;
	}

	if ($tag =~ m/^(acked-by|thanks-to|reviewed-by|noticed-by|tested-by|cc):/) {
		$signed="$signed$line";
		next;
	}

	# Keep review lines together with the signatures
	if ($line =~ m/^\[.*\@.*\:.*\]\n/) {
		$signed="$signed$line";
		next;
	}

	if ($tag =~ m/changeset:\s*(.*)\n/) {
		$num=$1;
	}

	if ($line =~ m|^(V4L\/DVB\s*\(.+\)\s*:.*\n)|) {
		$subject=$1;
		$sub_ok = 1;
		$line="\n";
	}

	if ($line =~ m/^#/) {
		next;
	}
	if ($sub_ok == 0) {
		if ($line =~ m/^\s*\n/) {
			next;
		}
		$sub_ok=1;
		if ($subject =~ m|V4L\/DVB\s*(.+)|) {
			$subject=$1;
		}
		$subject=$line;
		next;
	}

	if ($noblank) {
		if ($line =~ m/^\n/) {
			next;
		}
	}
	if (!$init) {
		$init=1;
		$noblank=0;
	}
	$body="$body$line";
}
close IN;

if ($from eq "") {
	print "Bad: author doesn't exist!\n";
	die;
}

if (!$maint_ok && $maintainer_name && $maintainer_email) {
	print "#No maintainer's signature. Adding it.\n";
	$signed=$signed."Signed-off-by: $maintainer_name <$maintainer_email>\n";
}

if (!$signed =~ m/$from/) {
	print "Bad: Author didn't signed his patch!\n";
	die;
}

$from=~s/^[\n\s]+//;
$from=~s/[\n\s]+$//;

$subject=~s/^[\n\s]+//;
$subject=~s/[\n\s]+$//;

$body=~s/^[\n]+//;
$body=~s/[\n\s]+$//;

if ($priority ne "") {
	$body="$body\n\n$priority";
}

$body="$body\n\n$signed";

$body=~s/^[\n\s]+//;
$body=~s/[\n\s]+$//;

# First from is used by hg to recognize commiter name
print "#Committer: $maintainer_name <$maintainer_email>\n";
print "$subject\n\n$from\n\n$body\n";
