#!/bin/sh

function xrmmod() {
	local module regex
	module="$1"
	regex=`echo $module | sed -e 's/[-_]/[-_]/g'`
	grep -qe "$regex" /proc/modules || return 0
	echo "unload $module"
	if test "$UID" = "0"; then
		/sbin/rmmod $module
	else
		sudo /sbin/rmmod $module
	fi
}

function xinsmod() {
	local module regex file args
	module="$1"
	shift
	args="$*"
	regex=`echo $module | sed -e 's/[-_]/[-_]/g'`
	grep -qe "$regex" /proc/modules && return
	file=""
	test -f "$module.o"	&& file="$module.o"
	test -f "$module.ko"	&& file="$module.ko"
	if test "$file" != ""; then
		echo "load $file $args"
		if test "$UID" = "0"; then
			/sbin/insmod $file $args
		else
			sudo /sbin/insmod $file $args
		fi
	else
		echo "load $module $args"
		if test "$UID" = "0"; then
			/sbin/modprobe $module $args
		else
			sudo /sbin/modprobe $module $args
		fi
	fi
}

function v4l2basic() {
	for module in	i2c-core i2c-algo-bit			\
			videodev v4l2-common v4l1-compat	\
			video-buf				\
			soundcore
	do
		xinsmod $module
	done
}
