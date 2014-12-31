#!/usr/bin/perl

my $diff = 'diff';
if ($ARGV[0] eq '-q') {
    $diff = 'qdiff';
    shift;
}
my $autopatch = shift;

# Get Hg username from environment
my $user = $ENV{HGUSER};

sub hgrcuser($)
{
	my $file = shift;
	my $ui = 0;
	open IN, '<', $file;
	while (<IN>) {
		$ui = 1 if (/^\s*\[ui\]/);
		if ($ui && /^\s*username\s*=\s*(\S.*?)\s*$/) {
			close IN;
			return($1);
		}
	}
	close IN;
	return("");
}

# Didn't work? Try the repo's .hgrc file
if ($user eq "") {
	my $hgroot = `hg root`;
	chomp($hgroot);
	$user = hgrcuser("$hgroot/.hg/hgrc");
}
# Ok, try ~/.hgrc next
if ($user eq "") {
	$user = hgrcuser("$ENV{HOME}/.hgrc");
}

# Still no luck? Try some other environment variables
if ($user eq "") {
	my $name = $ENV{CHANGE_LOG_NAME};
	my $email = $ENV{CHANGE_LOG_EMAIL_ADDRESS};
	$user = "$name <$email>" if ($name ne "" || $email ne "");
}

# Last try to come up with something
if ($user eq "") {
	$user = "$ENV{USER} <>";
}

$checkpatch=$ENV{CHECKPATCH};

if (!$checkpatch) {
	$checkpatch="/lib/modules/`uname -r`/build/scripts/checkpatch.pl";
}

my $cp_version;
open IN,"$checkpatch|";
while (<IN>) {
	tr/A-Z/a-z/;
	if (m/version\s*:\s*([\d\.]+)/) {
		$cp_version = $1;
	}
}
close IN;

my $intree_checkpatch = "scripts/checkpatch.pl --no-tree";
if (!open IN,"$intree_checkpatch|") {
	$intree_checkpatch = "v4l/".$intree_checkpatch;
	open IN,"$intree_checkpatch|";
}
while (<IN>) {
	tr/A-Z/a-z/;
	if (m/version\s*:\s*([\d\.]+)/) {
		if ($1 > $cp_version) {
			print "# WARNING: $checkpatch version $cp_version is\n"
			      ."#         older than $intree_checkpatch version"
			      ." $1.\n#          Using in-tree one.\n#\n";
			$cp_version = $1;
			$checkpatch = $intree_checkpatch;
		}
	}
}
close IN;

print "# Added/removed/changed files:\n";
system "hg $diff | diffstat -p1 -c";

open IN,"hg $diff | $checkpatch -q --nosignoff -|";
my $err=0;
while (<IN>) {
	if (!$err) {
		print "#\n# WARNING: $checkpatch version $cp_version returned ".
		      "some errors.\n#          Please fix.\n#\n";


		$err=1;
	}
	print "# $_";
}
close IN;


if (-s $autopatch) {
	print "#\n# Note, a problem with your patch was detected!  These changes were made\n";
	print "# automatically: $autopatch\n";
	system "diffstat -p0 -c $autopatch";
	print "#\n# Please review these changes and see if they belong in your patch or not.\n";
}
if ($diff eq 'qdiff') {
	# Use existing mq patch logfile?
	open IN, "hg qheader |";
	my @header = <IN>;
	close IN;

	if ($#header > 0) {
		# Use existing header
		print @header;
		exit;
	}
	# No header, use pre-made log message below

	# Hg will strip lines that start with "From: " from mq patch headers!
	# In order to stop it, we insert this extra From line at the top,
	# Hg will strip it and then leave the real from line alone.
	print "From: $user\n\n";
}
print <<"EOF";
#
# Patch Subject (a brief description with less than 74 chars):


# From Line, identifying the name of the patch author
From: $user

# A detailed description:

# NEW: Please change the priority of the patch to "high" if the patch is
#      a bug fix, or are meant to be applied at the first upstream
#      version of a new driver whose changes don't depend on changes on
#      core modules
Priority: normal

# At the end Signed-off-by: fields by patch author and committer, at least.
Signed-off-by: $user
EOF
