/* test_pes.c - Test for PES filters.
 * usage: DEMUX=/dev/dvb/adapterX/demuxX test_pes PID
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
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <linux/dvb/dmx.h>

#include "hex_dump.h"

#define MAX_PES_SIZE (4*1024)


void usage(void)
{
	fprintf(stderr, "usage: test_pes PID [filename]\n");
	fprintf(stderr, "       Print a hexdump of PES packets from PID to stdout.\n");
	fprintf(stderr, "  filename : Write binary PES data to file (no hexdump).\n");
	fprintf(stderr, "       The default demux device used can be changed\n");
	fprintf(stderr, "       using the DEMUX environment variable\n");
	exit(1);
}

void process_pes(int fd, FILE *out)
{
	uint8_t buf[MAX_PES_SIZE];
	int bytes;

	bytes = read(fd, buf, sizeof(buf));
	if (bytes < 0) {
		if (errno == EOVERFLOW) {
			fprintf(stderr, "read error: buffer overflow (%d)\n",
					EOVERFLOW);
			return;
		}
		else {
			perror("read");
			exit(1);
		}
	}
	if (out == stdout) {
		hex_dump(buf, bytes);
		printf("\n");
	}
	else {
		printf("got %d bytes\n", bytes);
		if (fwrite(buf, 1, bytes, out) == 0)
			perror("write output");
	}
}

int set_filter(int fd, unsigned int pid)
{
	struct dmx_pes_filter_params f;

	f.pid = (uint16_t) pid;
	f.input = DMX_IN_FRONTEND;
	f.output = DMX_OUT_TAP;
	f.pes_type = DMX_PES_OTHER;
	f.flags = DMX_IMMEDIATE_START;
	if (ioctl(fd, DMX_SET_PES_FILTER, &f) == -1) {
		perror("ioctl DMX_SET_PES_FILTER");
		return 1;
	}
	return 0;
}


int main(int argc, char *argv[])
{
	int dmxfd;
	unsigned long pid;
	char *dmxdev = "/dev/dvb/adapter0/demux0";
	FILE *out = stdout;

	if (argc != 2 && argc != 3)
		usage();

	pid = strtoul(argv[1], NULL, 0);
	if (pid > 0x1fff)
		usage();
	if (getenv("DEMUX"))
		dmxdev = getenv("DEMUX");

	fprintf(stderr, "test_pes: using '%s'\n", dmxdev);
	fprintf(stderr, "          PID 0x%04lx\n", pid);

	if (argc == 3) {
		out = fopen(argv[2], "wb");
		if (!out) {
			perror("open output file");
			exit(1);
		}
		fprintf(stderr, "          output to '%s'\n", argv[2]);
	}

	if ((dmxfd = open(dmxdev, O_RDWR)) < 0){
		perror("open");
		return 1;
	}

	if (set_filter(dmxfd, pid) != 0)
		return 1;

	for (;;) {
		process_pes(dmxfd, out);
	}

	close(dmxfd);
	return 0;
}
