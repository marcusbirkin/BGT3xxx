#!/usr/bin/perl

# Copyright (C) 2006 Trent Piepho <xyzzy@speakeasy.org>
# Automatic pull request generator

# Generates a pull request for all changesets in current Hg repository, that
# do not appear in a remote repository.
#
# There are _three_ repositories involved in this operation.
#
# The first is the remote repository that you want your changes to be pulled
# into.  The default is the master repository at
# http://linuxtv.org/hg/v4l-dvb, but you can specify something else if you
# want.  This repository is called the "to repo".
#
# The next two repositories should be copies of each other.  The first is your
# local repository.  You must run this script from some directory in that
# repository.  The second repository is a remote copy on some public server,
# e.g. linuxtv.org.  This is called the "from repo".  The default for the from
# repo is the "default-push" path from the Hg configuration, which is where hg
# will push to if you don't specify a path to "hg push".  The script will do
# some checks to make sure these two repositories are copies of each other.
#
# The repository used to the changesets are comming from is local repository,
# due to Mercurial's limitations.  However, links to your local repository
# would be useless in a pull request, because no one else can see your local
# repository.  So, the links are change to use the "from repo" instead.  This
# is why your local repo and the from repo must be copies of each other.
# Running "hg push" before generating the pull request should be enough.

$maintainer	= 'Mauro';
$to_repo 	= 'http://linuxtv.org/hg/v4l-dvb';
# Default for when a default-push path wasn't defined
$from_repo_base = 'http://linuxtv.org/hg/~username/';

# What to open for the pull request
$output_file	= '>&STDOUT';
# Example, to a local file: '>pull_req'
# Example, file on remote host: '|ssh remote.host.com cat \\> pull_req'

# Text of the pull request.  $nstr is e.g. "changeset" or "42 changesets"
$salutation     = <<'EOF';
$maintainer,

Please pull from $from_repo

for the following $nstr:

EOF

# The closing of the request, name and fname taken from hg username setting
$valediction	= <<'EOF';

Thanks,
$fname
EOF

if($#ARGV < 0) {
    if(`hg showconfig paths` =~ m/^paths\.default-push=(.*)$/m) {
	$from_repo = $1;
	$from_repo =~ s/^ssh:/http:/;
    } else {
	`hg root` =~ m|/([^/]+)\n$|;
	my $repo = $1;
	$from_repo = $from_repo_base . $repo;
    }
}

$from_repo = $ARGV[0] if($#ARGV >= 0);
$to_repo = $ARGV[1] if($#ARGV >= 1);

open OUT, $output_file or die "Opening: $!";

if (`hg outgoing $from_repo` !~ /^no changes found$/m ||
    `hg incoming $from_repo` !~ /^no changes found$/m) {
    my $cur = `hg root`; chomp $cur;
    print "$cur and $from_repo do not match!\n";
    print "Prehaps you forgot to push your changes?\n";
    exit;
}

open IN, "hg outgoing -M $to_repo |";
while(<IN>) {
    if(/^changeset:\s+\d+:([[:xdigit:]]{12})$/) {
	push @changesets, $1;
    } elsif(/^summary:\s+(\S.*)$/) {
	if ($1 =~ /^merge:/) {
	    # Skip merge changesets
	    pop @changesets;
	} else {
	    push @summaries, $1;
	}
    }
}
close IN;
$#changesets == $#summaries or die "Confused by hg outgoing output";

foreach (0 .. $#summaries) {
    if($summaries[$_] =~ /^merge:/) {
	splice @summaries, $_, 1;
	splice @changesets, $_, 1;
    }
}

$n = $#changesets + 1;
$n > 0 or die "Nothing to pull!";
$nstr = ($n==1)?"changeset":"$n changesets";

print OUT eval qq("$salutation");

for (0 .. $#summaries) {
    printf OUT "%02d/%02d: $summaries[$_]\n", $_+1, $n;
    print OUT "$from_repo?cmd=changeset;node=$changesets[$_]\n";
    print OUT "\n";
}

print OUT "\n";

open IN, 'hg export ' . join(' ', @changesets) . '| diffstat |';
print OUT while(<IN>);
close IN;

`hg showconfig ui.username` =~ m/((\S+).*)\s+</;
my $name = $1; my $fname = $2;
print OUT eval qq("$valediction");
