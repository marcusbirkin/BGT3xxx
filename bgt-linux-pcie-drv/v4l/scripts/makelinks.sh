#!/bin/sh
# link drivers sources from CVS or release tarball into your 2.6.x kernel sources;

if test -z $1 || ! test -d $1 ; then
	echo
	echo "  usage: $0 <path_to_kernel_to_patch>"
	echo
	exit
fi

echo "patching $1..."

cd linux
PWD=`pwd`
FINDDIR="-name CVS -prune -o -type d"
FINDFILE="-name CVS -prune -o -type f -not -name .cvsignore\
	-not -name '*.rej' -not -name '*.orig'"

for dir in drivers include Documentation; do
	find $dir $FINDDIR -exec mkdir -p -v $1/{} \;
	find $dir $FINDFILE -exec ln -f -s $PWD/{} $1/{} \;
done

for dir in drivers/media include; do
	find $dir $FINDDIR -exec ln -f -s $PWD/../v4l/compat.h $1/{} \; \
		-exec touch $1/{}/config-compat.h \;
done

cat > $1/drivers/media/Kbuild <<EOF
EXTRA_CFLAGS+=-include include/linux/version.h
export EXTRA_CFLAGS
include drivers/media/Makefile
EOF
