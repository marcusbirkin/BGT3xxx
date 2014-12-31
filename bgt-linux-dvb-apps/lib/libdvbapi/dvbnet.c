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
#include <linux/dvb/net.h>
#include <errno.h>
#include "dvbnet.h"

int dvbnet_open(int adapter, int netdeviceid)
{
	char filename[PATH_MAX+1];
	int fd;

	sprintf(filename, "/dev/dvb/adapter%i/net%i", adapter, netdeviceid);
	if ((fd = open(filename, O_RDWR)) < 0) {
		// if that failed, try a flat /dev structure
		sprintf(filename, "/dev/dvb%i.net%i", adapter, netdeviceid);
		fd = open(filename, O_RDWR);
	}

	return fd;
}

int dvbnet_add_interface(int fd, uint16_t pid, enum dvbnet_encap encapsulation)
{
	struct dvb_net_if params;
	int status;

	memset(&params, 0, sizeof(params));
	params.pid = pid;

	switch(encapsulation) {
	case DVBNET_ENCAP_MPE:
		params.feedtype = DVB_NET_FEEDTYPE_MPE;
		break;

	case DVBNET_ENCAP_ULE:
		params.feedtype = DVB_NET_FEEDTYPE_ULE;
		break;

	default:
		return -EINVAL;
	}

	status = ioctl(fd, NET_ADD_IF, &params);
	if (status < 0)
		return status;
	return params.if_num;
}

int dvbnet_get_interface(int fd, int ifnum, uint16_t *pid, enum dvbnet_encap *encapsulation)
{
	struct dvb_net_if info;
	int res;

	memset(&info, 0, sizeof(struct dvb_net_if));
	info.if_num = ifnum;

	if ((res = ioctl(fd, NET_GET_IF, &info)) < 0)
		return res;

	*pid = info.pid;
	switch(info.feedtype) {
	case DVB_NET_FEEDTYPE_MPE:
		*encapsulation = DVBNET_ENCAP_MPE;
		break;

	case DVB_NET_FEEDTYPE_ULE:
		*encapsulation = DVBNET_ENCAP_ULE;
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

int dvbnet_remove_interface(int fd, int ifnum)
{
	return ioctl(fd, NET_REMOVE_IF, ifnum);
}
