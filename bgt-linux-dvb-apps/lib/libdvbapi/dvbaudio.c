/*
 * libdvbnet - a DVB network support library
 *
 * Copyright (C) 2005 Andrew de Quincey (adq_dvb@lidskialf.net)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/dvb/audio.h>
#include <errno.h>
#include "dvbaudio.h"

int dvbaudio_open(int adapter, int audiodeviceid)
{
	char filename[PATH_MAX+1];
	int fd;

	sprintf(filename, "/dev/dvb/adapter%i/audio%i", adapter, audiodeviceid);
	if ((fd = open(filename, O_RDWR)) < 0) {
		// if that failed, try a flat /dev structure
		sprintf(filename, "/dev/dvb%i.audio%i", adapter, audiodeviceid);
		fd = open(filename, O_RDWR);
	}

	return fd;
}

int dvbaudio_set_bypass(int fd, int bypass)
{
	return ioctl(fd, AUDIO_SET_BYPASS_MODE, bypass);
}
