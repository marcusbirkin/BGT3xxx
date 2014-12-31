#!/bin/sh

# config
release="$HOME/kernel/release"
ver_bt="0.9.15"
ver_sa="0.2.12"
ver_cx="0.0.4"

# common files
files_v4l="v4l*.[ch] video-buf.[ch] videodev*.h"
files_tuner="tuner.[ch] tda9887.[ch]"
files_i2c="id.h audiochip.h"
files_common="$files_v4l $files_tuner $files_i2c doc"

# other files
files_ir="ir-common.[ch]"
files_audio="msp3400.[ch] tvaudio.[ch]"

files_bttv="bt848.h btcx*.[ch] bttv*.[ch] ir-kbd*.c"
files_saa="saa7134*.[ch] saa6752hs.[ch] ir-kbd-i2c.c"
files_cx="btcx*.[ch] cx*.[ch]"


######################################################################################
# helpers

function build_release () {
	local name="$1"; shift
	local version="$1"; shift
	local files="$*"
	local dest="$WORK/$name-$version"
	local tarball="$release/$name-$version.tar.gz"

	# copy / prepare stuff
	mkdir "$dest"
	cp -av $files "$dest" || exit 1
	cp -v Makefile "$dest"
	cp -v "scripts/config.$name" "$dest"/Make.config

	# build test
	(cd $dest; make) || exit 1
#	(cd $dest; ls *.o; sleep 5)
	(cd $dest; make clean)

	# build tarball
	tar czCf "$WORK" "$tarball" "$name-$version"
}


######################################################################################
# main

# tmp dir for my files
WORK="${TMPDIR-/tmp}/${0##*/}-$$"
mkdir "$WORK" || exit 1
trap 'rm -rf "$WORK"' EXIT

build_release "bttv" "$ver_bt" \
	"$files_common" "$files_ir" "$files_audio" "$files_bttv"
build_release "saa7134" "$ver_sa" \
	"$files_common" "$files_ir" "$files_audio" "$files_saa"
build_release "cx88" "$ver_cx" \
	"$files_common" "$files_cx"
