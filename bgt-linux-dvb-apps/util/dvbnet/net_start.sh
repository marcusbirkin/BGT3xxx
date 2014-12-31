#!/bin/sh

PID=0x511
DEV_NAME=dvb0_0
IP_ADDR=10.1.1.1

./dvbnet -p $PID

/sbin/ifconfig $DEV_NAME $IP_ADDR

#
#  you can reconfigure the MAC adress like this:
#
#MAC_ADDR=00:01:02:03:04:05
#/sbin/ifconfig $DEV_NAME hw ether $MAC_ADDR
