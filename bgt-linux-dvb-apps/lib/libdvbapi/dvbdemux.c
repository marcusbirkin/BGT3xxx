/*
 * libdvbdemux - a DVB demux library
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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <linux/dvb/dmx.h>
#include "dvbdemux.h"


int dvbdemux_open_demux(int adapter, int demuxdevice, int nonblocking)
{
	char filename[PATH_MAX+1];
	int flags = O_RDWR;
	int fd;

	if (nonblocking)
		flags |= O_NONBLOCK;

	sprintf(filename, "/dev/dvb/adapter%i/demux%i", adapter, demuxdevice);
	if ((fd = open(filename, flags)) < 0) {
		// if that failed, try a flat /dev structure
		sprintf(filename, "/dev/dvb%i.demux%i", adapter, demuxdevice);
		fd = open(filename, flags);
	}

	return fd;
}

int dvbdemux_open_dvr(int adapter, int dvrdevice, int readonly, int nonblocking)
{
	char filename[PATH_MAX+1];
	int flags = O_RDWR;
	int fd;

	if (readonly)
		flags = O_RDONLY;
	if (nonblocking)
		flags |= O_NONBLOCK;

	sprintf(filename, "/dev/dvb/adapter%i/dvr%i", adapter, dvrdevice);
	if ((fd = open(filename, flags)) < 0) {
		// if that failed, try a flat /dev structure
		sprintf(filename, "/dev/dvb%i.dvr%i", adapter, dvrdevice);
		fd = open(filename, flags);
	}

	return fd;
}

int dvbdemux_set_section_filter(int fd, int pid,
				uint8_t filter[18], uint8_t mask[18],
				int start, int checkcrc)
{
	struct dmx_sct_filter_params sctfilter;

	memset(&sctfilter, 0, sizeof(sctfilter));
	sctfilter.pid = pid;
	memcpy(sctfilter.filter.filter, filter, 1);
	memcpy(sctfilter.filter.filter+1, filter+3, 15);
	memcpy(sctfilter.filter.mask, mask, 1);
	memcpy(sctfilter.filter.mask+1, mask+3, 15);
	memset(sctfilter.filter.mode, 0, 16);
	if (start)
		sctfilter.flags |= DMX_IMMEDIATE_START;
	if (checkcrc)
		sctfilter.flags |= DMX_CHECK_CRC;

	return ioctl(fd, DMX_SET_FILTER, &sctfilter);
}

int dvbdemux_set_pes_filter(int fd, int pid,
			    int input, int output,
			    int pestype,
			    int start)
{
	struct dmx_pes_filter_params filter;

	memset(&filter, 0, sizeof(filter));
	filter.pid = pid;

	switch(input) {
	case DVBDEMUX_INPUT_FRONTEND:
		filter.input = DMX_IN_FRONTEND;
		break;

	case DVBDEMUX_INPUT_DVR:
		filter.input = DMX_IN_DVR;
		break;

	default:
		return -EINVAL;
	}

	switch(output) {
	case DVBDEMUX_OUTPUT_DECODER:
		filter.output = DMX_OUT_DECODER;
		break;

	case DVBDEMUX_OUTPUT_DEMUX:
		filter.output = DMX_OUT_TAP;
		break;

	case DVBDEMUX_OUTPUT_DVR:
		filter.output = DMX_OUT_TS_TAP;
		break;

#ifdef DMX_OUT_TSDEMUX_TAP
	case DVBDEMUX_OUTPUT_TS_DEMUX:
		filter.output = DMX_OUT_TSDEMUX_TAP;
		break;
#endif

	default:
		return -EINVAL;
	}

	switch(pestype) {
	case DVBDEMUX_PESTYPE_AUDIO:
		filter.pes_type = DMX_PES_AUDIO;
		break;

	case DVBDEMUX_PESTYPE_VIDEO:
		filter.pes_type = DMX_PES_VIDEO;
		break;

	case DVBDEMUX_PESTYPE_TELETEXT:
		filter.pes_type = DMX_PES_TELETEXT;
		break;

	case DVBDEMUX_PESTYPE_SUBTITLE:
		filter.pes_type = DMX_PES_SUBTITLE;
		break;

	case DVBDEMUX_PESTYPE_PCR:
		filter.pes_type = DMX_PES_PCR;
		break;

	default:
		return -EINVAL;
	}

	if (start)
		filter.flags |= DMX_IMMEDIATE_START;

	return ioctl(fd, DMX_SET_PES_FILTER, &filter);
}

int dvbdemux_set_pid_filter(int fd, int pid,
			    int input, int output,
			    int start)
{
	struct dmx_pes_filter_params filter;

	memset(&filter, 0, sizeof(filter));
	if (pid == -1)
		filter.pid = 0x2000;
	else
		filter.pid = pid;

	switch(input) {
	case DVBDEMUX_INPUT_FRONTEND:
		filter.input = DMX_IN_FRONTEND;
		break;

	case DVBDEMUX_INPUT_DVR:
		filter.input = DMX_IN_DVR;
		break;

	default:
		return -EINVAL;
	}

	switch(output) {
	case DVBDEMUX_OUTPUT_DECODER:
		filter.output = DMX_OUT_DECODER;
		break;

	case DVBDEMUX_OUTPUT_DEMUX:
		filter.output = DMX_OUT_TAP;
		break;

	case DVBDEMUX_OUTPUT_DVR:
		filter.output = DMX_OUT_TS_TAP;
		break;

#ifdef DMX_OUT_TSDEMUX_TAP
	case DVBDEMUX_OUTPUT_TS_DEMUX:
		filter.output = DMX_OUT_TSDEMUX_TAP;
		break;
#endif

	default:
		return -EINVAL;
	}

	filter.pes_type = DMX_PES_OTHER;

	if (start)
		filter.flags |= DMX_IMMEDIATE_START;

	return ioctl(fd, DMX_SET_PES_FILTER, &filter);
}

int dvbdemux_start(int fd)
{
	return ioctl(fd, DMX_START);
}

int dvbdemux_stop(int fd)
{
	return ioctl(fd, DMX_STOP);
}

int dvbdemux_get_stc(int fd, uint64_t *stc)
{
	struct dmx_stc _stc;
	int result;

	memset(stc, 0, sizeof(_stc));
	if ((result = ioctl(fd, DMX_GET_STC, &_stc)) != 0) {
		return result;
	}

	*stc = _stc.stc / _stc.base;
	return 0;
}

int dvbdemux_set_buffer(int fd, int bufsize)
{
	return ioctl(fd, DMX_SET_BUFFER_SIZE, bufsize);
}
