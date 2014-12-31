#!/bin/bash

EDITOR=$1
WHITESPCE=$2

if [ "$WHITESPCE" == "" ]; then
	exit 13
fi

TMPMSG=$1

scripts/cardlist
scripts/prep_commit_msg.pl $WHITESPCE >  $TMPMSG

#trap 'rm -rf $TMPMSG' EXIT

CHECKSUM=`md5sum "$TMPMSG"`
$EDITOR $TMPMSG || exit $?
echo "$CHECKSUM" | md5sum -c --status && echo "*** commit message not changed. Aborting. ***"  && exit 13
DATE="`scripts/hghead.pl $TMPMSG|perl -ne 'if (m/\#[dD]ate:\s+(.*)/) { print $1; }'`"

if [ "$DATE" != "" ]; then
	echo Patch date is $DATE
	scripts/hghead.pl $TMPMSG| grep -v '^#' | hg commit -d "$DATE" -l -
else
	scripts/hghead.pl $TMPMSG| grep -v '^#' | hg commit -l -
fi

if [ "$?" != "0" ]; then
	echo "Couldn't apply the patch"
	exit 13
fi

echo "*** PLEASE CHECK IF LOG IS OK:"
hg log -v -r -1
echo "*** If not ok, do \"hg rollback\" and \"make commit\" again"
