#!/bin/bash
#
# Copyright (C) 2006 Markus Rechberger <mrechberger@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

which /usr/bin/dialog >/dev/null
if [ "0" != "$?" ]; then
echo "this tool requires  \"dialog\" (http://hightek.org/dialog/)"
exit 1
fi
uid=`id -u`
if [ "0" != "$uid" ]; then
echo "this tool must be run as root, you can disable this message by editing the script but only do that unless you know what you're doing!"
exit 1
fi
which gcc > /dev/null
if [ "0" != "$?" ]; then
echo "this tool won't work unless you install gcc"
exit 1
fi
test -f ossid
if [ "0" != "$?" ]; then
cat > ossid.c <<_EOF
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/soundcard.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv){
        int fd;
        struct mixer_info *info;
        fd=open(argv[1],O_RDONLY);
        if(fd>=0){
                info=malloc(sizeof(struct mixer_info));
                ioctl(fd,SOUND_MIXER_INFO,info);
                printf("%c \"%s %s\"\n",(argv[1][strlen(argv[1])-1]=='p')?'0':(argv[1][strlen(argv[1])-1]+1),info->name,info->id);
                free(info);
                close(fd);
        } else {
                return 1;
        }
        return 0;
}
_EOF
gcc ossid.c -o ossid
rm ossid.c
fi

test -f /proc/asound/cards
if [ "0" != "$?" ]; then
dialog --title "Welcome" --backtitle "Empia Sound Configuration" \
--msgbox "Your system doesn't support ALSA, please have a look at \
www.alsa-project.org and set it up properly \


Press any key to continue... " 11 50
exit 1;
fi

dialog --title "Welcome" --backtitle "Empia Sound Configuration" \
--msgbox "This tool was written to ease up sound configuration for
* Terratec Hybrid XS
* Terratec Cinergy 250 USB 2.0
* Hauppauge HVR 900
* and possible others :)

first select an usb audio source, as target choose your soundcard
Press any key to continue... " 13 60

ls /dev/dsp* | while read a; do ./ossid $a; done | xargs dialog --menu "Choose your TV Audio source:" 12 60 5 2>/tmp/em2880_source.$$
ls /dev/dsp* | while read a; do ./ossid $a; done | xargs dialog --menu "Choose your output soundcard device:" 12 60 5 2>/tmp/em2880_dst.$$

source=`egrep '^[0-9p]' /tmp/em2880_source.$$`
dst=`egrep '^[0-9p]' /tmp/em2880_dst.$$`

echo "playing $source to $dst";
if [ "$source" = "0" ]; then
	device="/dev/dsp"
else
	device="/dev/dsp`expr $source - 1`"
fi

if [ "$dst" = "0" ]; then
	device2="/dev/dsp"
else
	device2="/dev/dsp`expr $source - 1`"
fi
clear
echo "Using command: sox -r 48000 -w -c 2 -t ossdsp $device -t ossdsp $device2"
sox -r 48000 -w -c 2 -t ossdsp $device -t ossdsp $device2
