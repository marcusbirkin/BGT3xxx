#!/usr/bin/perl

my $merge_tree=shift or die "Should specify the pulled tree";

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

sub check_heads()
{
	my $count=0;
	open IN, 'hg heads|';
	while (<IN>) {
		if (m/^[Cc]hangeset:/) {
			$count++;
		}
	}
	close IN;
	return $count;
}

sub curr_changeset()
{
	my $changeset = -1;

	open IN, 'hg heads|';
	while (<IN>) {
		if (m/^[Cc]hangeset:\s*(\d+)/) {
			if ($changeset < 0) {
				$changeset = $1;
			} else {
				if ($1 < $changeset) {
					$changeset = $1;
				}
			}
		}
	}
	close IN;
	return $changeset;
}

sub check_status()
{
	my $count=0;
	open IN, 'hg status -m -a -d -r|';
	while (<IN>) {
		$count++;
	}
	close IN;
	return $count;
}

sub rollback()
{
	print "*** ERROR *** Rolling back hg pull $merge_tree\n";
	system("hg rollback");
	system("hg update -C");
	exit -1;
}

####################
# Determine username

# Get Hg username from environment
my $user = $ENV{HGUSER};

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
	print "*** ERROR *** User not known. Can't procceed\n";
	exit -1;
}

######################
# Do some sanity tests

print "Checking if everything is ok, before applying the new tree.\n";

my $n_heads = check_heads();
die "Your tree currently have more than one head (it has $n_heads heads). Can't procceed\n" if ($n_heads > 1);

my $dirty = check_status();
die "Your tree currently has changes. Can't procceed\n" if ($dirty);

my $curr_cs = curr_changeset();

###########
# Pull tree

print "hg pull $merge_tree\n";

my $ret = system("hg pull $merge_tree");
die "Couldn't pull from $merge_tree\n" if ($ret);

#############################
# Merge and commit, if needed

$n_heads = check_heads();
if ($n_heads > 2) {
	print "The merged tree have more than one head (it has $n_heads heads). Can't procceed.\n";
	rollback();
}

if ($n_heads == 2) {
	print "Merging the new changesets\n";

	$ret = system("hg merge");
	if ($ret) {
		print "hg merge failed. Can't procceed.\n";
		rollback();
	}

	print "Committing the new tree\n";
	# Write the commit message
	$msg= "merge: $merge_tree\n\nFrom: $user\n\nSigned-off-by: $user\n";
	$ret=system("hg commit -m '$msg'");
	if ($ret) {
		print "hg commit failed. Can't procceed.\n";
		rollback();
	}
}

#####################
# Test resulting tree

print "Testing if the build didn't break compilation. Only errors and warnings will be displayed. Please wait.\n";
$ret = system ('make allmodconfig');
if (!ret) {
	$ret = system ('make mismatch|egrep -v "^\s*CC"|egrep -v "^\s*LD"');
}
if ($ret) {
	print "*** ERROR *** Build failed. Can't procceed.\n";

	# To avoid the risk of doing something really bad, let's ask the user to run hg strip
	print "Your tree is dirty. Since hg has only one rollback level, you'll need to use, instead:";
	print "\thg strip $curr_cs; hg update -C";
	print "You'll need to have hg mq extension enabled for hg strip to work.\n";

	exit -1;
}

##############################
# Everything is ok, let's push

print "Pushing the new tree at the remote repository specified at .hg/hgrc\n";
$ret=system ("hg push");
if ($ret) {
	print "hg push failed. Don't forget to do the push later.\n";
}
