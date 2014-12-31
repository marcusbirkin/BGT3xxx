/* test_sec_ne.c - Test for Not-Equal section filters.
 * usage: DEMUX=/dev/dvb/adapterX/demuxX test_sec_ne pid [tid [tid_ext]]
 *
 * Copyright (C) 2002 convergence GmbH
 * Johannes Stezenbach <js@convergence.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <linux/dvb/dmx.h>

#include "hex_dump.h"

#define MAX_SECTION_SIZE 8192

static unsigned int version_number;

void usage(void)
{
	fprintf(stderr, "usage: test_sec_ne pid [tid [tid_ext]]\n");
	fprintf(stderr, "       Read the version_number from the first section\n");
	fprintf(stderr, "       and sec a filter to wait for it to change.\n");
	fprintf(stderr, "       The default demux device used can be changed\n");
	fprintf(stderr, "       using the DEMUX environment variable;\n");
	exit(1);
}


void process_section(int fd)
{
	uint8_t buf[MAX_SECTION_SIZE];
	int bytes;

	bytes = read(fd, buf, sizeof(buf));
	if (bytes < 0) {
		perror("read");
		if (bytes != ETIMEDOUT)
			exit(1);
	}
	hex_dump(buf, bytes);
	version_number = ((buf[5] >> 1) & 0x1f);
	printf("current version_number: %u\n\n", version_number);
}

int set_filter(int fd, unsigned int pid, unsigned int tid,
		unsigned int tid_ext, unsigned int _version_number)
{
	struct dmx_sct_filter_params f;
	unsigned long bufsz;

	if (getenv("BUFFER")) {
		bufsz=strtoul(getenv("BUFFER"), NULL, 0);
		if (bufsz > 0 && bufsz <= MAX_SECTION_SIZE) {
			fprintf(stderr, "DMX_SET_BUFFER_SIZE %lu\n", bufsz);
			if (ioctl(fd, DMX_SET_BUFFER_SIZE, bufsz) == -1) {
				perror("DMX_SET_BUFFER_SIZE");
				return 1;
			}
		}
	}
	memset(&f.filter, 0, sizeof(struct dmx_filter));
	f.pid = (uint16_t) pid;
	if (tid < 0x100) {
		f.filter.filter[0] = (uint8_t) tid;
		f.filter.mask[0]   = 0xff;
	}
	if (tid_ext < 0x10000) {
		f.filter.filter[1] = (uint8_t) (tid_ext >> 8);
		f.filter.filter[2] = (uint8_t) (tid_ext & 0xff);
		f.filter.mask[1]   = 0xff;
		f.filter.mask[2]   = 0xff;
	}
	if (_version_number < 0x20) {
		f.filter.filter[3] = (uint8_t) (_version_number << 1);
		f.filter.mask[3]   = 0x3e;
		f.filter.mode[3]   = 0x3e;
	}
	f.timeout = 0;
	f.flags = DMX_IMMEDIATE_START;

	if (ioctl(fd, DMX_SET_FILTER, &f) == -1) {
		perror("DMX_SET_FILTER");
		return 1;
	}
	return 0;
}


int main(int argc, char *argv[])
{
	int dmxfd;
	unsigned long pid, tid, tid_ext;
	char * dmxdev = "/dev/dvb/adapter0/demux0";

	if (argc < 2 || argc > 4)
		usage();

	pid = strtoul(argv[1], NULL, 0);
	if (pid > 0x1fff)
		usage();
	if (argc > 2) {
		tid = strtoul(argv[2], NULL, 0);
		if (tid > 0xff)
			usage();
	} else
		tid = 0x100;
	if (argc > 3) {
		tid_ext = strtoul(argv[3], NULL, 0);
		if (tid_ext > 0xffff)
			usage();
	} else
		tid_ext = 0x10000;

	if (getenv("DEMUX"))
		dmxdev = getenv("DEMUX");
	fprintf(stderr, "test_sec_ne: using '%s'\n", dmxdev);
	fprintf(stderr, "             PID 0x%04lx\n", pid);
	if (tid < 0x100)
		fprintf(stderr, "               TID 0x%02lx\n", tid);
	if (tid_ext < 0x10000)
		fprintf(stderr, "               TID EXT 0x%04lx\n", tid_ext);

	if ((dmxfd = open(dmxdev, O_RDWR)) < 0){
		perror("open");
		return 1;
	}

	if (set_filter(dmxfd, pid, tid, tid_ext, 0xff) != 0)
		return 1;
	process_section(dmxfd);
	while (1) {
		if (set_filter(dmxfd, pid, tid, tid_ext, version_number) != 0)
			return 1;
		process_section(dmxfd);
	}
	for (;;) {
		process_section(dmxfd);
	}

	close(dmxfd);
	return 0;
}
