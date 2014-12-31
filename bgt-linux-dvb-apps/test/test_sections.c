/* test_sections.c - Test for section filters.
 * usage: DEMUX=/dev/dvb/adapterX/demuxX test_sections [PID [TID]]
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


#if !defined(DMX_FILTER_SIZE)
#define DMX_FILTER_SIZE 16
#endif

static void parse_filter(unsigned char* filter, const char* filter_desc)
{
        int filter_idx;
        char* end_ptr;

        memset(filter, '\0', DMX_FILTER_SIZE);

        for (filter_idx = 1; /* Leave first byte for tid */
             filter_idx < DMX_FILTER_SIZE-1; ++filter_idx) {
                filter[filter_idx] = strtoul(filter_desc, &end_ptr, 0);

                if (*end_ptr == '\0' || end_ptr == filter_desc)
                        break;

                filter_desc = end_ptr;
        }
}

void usage(void)
{
	fprintf(stderr, "usage: test_sections PID [TID] [FILTER] [MASK]\n");
	fprintf(stderr, "       The default demux device used can be changed\n");
	fprintf(stderr, "       using the DEMUX environment variable;\n");
	fprintf(stderr, "       set env BUFFER to play with DMX_SET_BUFFER_SIZE\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "       The optional filter and mask may be used to filter on\n");
        fprintf(stderr, "       additional bytes. These bytes may be given in hex or dec\n");
        fprintf(stderr, "       separated by spaces (hint: you may have to use quotes around\n");
        fprintf(stderr, "       FILTER and MASK). TID is always the first byte of the filter,\n");
        fprintf(stderr, "       and the first byte from FILTER applies to byte 3 of the section\n");
        fprintf(stderr, "       data (i.e. the first byte after the section length)\n");
	exit(1);
}


void process_section(int fd)
{
	uint8_t buf[MAX_SECTION_SIZE];
	int bytes;

	bytes = read(fd, buf, sizeof(buf));
	if (bytes < 0) {
		perror("read");
		if (errno != EOVERFLOW)
			exit(1);
	}
	hex_dump(buf, bytes);
	printf("\n");
}

int set_filter(int fd, unsigned int pid,
               const unsigned char* filter, const unsigned char* mask)
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

        memcpy(f.filter.filter, filter, DMX_FILTER_SIZE);
        memcpy(f.filter.mask, mask, DMX_FILTER_SIZE);
	f.timeout = 0;
	f.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

	if (ioctl(fd, DMX_SET_FILTER, &f) == -1) {
		perror("DMX_SET_FILTER");
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int dmxfd;
	unsigned long pid, tid;
	char * dmxdev = "/dev/dvb/adapter0/demux0";
        unsigned char filter[DMX_FILTER_SIZE];
        unsigned char mask[DMX_FILTER_SIZE];
        int filter_idx;

	if (argc < 2 || argc > 5)
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

        if (argc > 3)
                parse_filter(filter, argv[3]);
        else
                memset(filter, '\0', sizeof(filter));

        if (argc > 4)
                parse_filter(mask, argv[4]);
        else
                memset(mask, '\0', sizeof(mask));

        if (tid < 0x100) {
                filter[0] = tid;
                mask[0]   = 0xff;
        }

	if (getenv("DEMUX"))
		dmxdev = getenv("DEMUX");
	fprintf(stderr, "test_sections: using '%s'\n", dmxdev);
	fprintf(stderr, "  PID 0x%04lx\n", pid);
	if (tid < 0x100)
		fprintf(stderr, "  TID 0x%02lx\n", tid);


        fprintf(stderr, "  Filter ");

        for (filter_idx = 0; filter_idx < DMX_FILTER_SIZE; ++filter_idx)
                fprintf(stderr, "0x%.2x ", filter[filter_idx]);
        fprintf(stderr, "\n");

        fprintf(stderr, "    Mask ");

        for (filter_idx = 0; filter_idx < DMX_FILTER_SIZE; ++filter_idx)
                fprintf(stderr, "0x%.2x ", mask[filter_idx]);

        fprintf(stderr, "\n");

	if ((dmxfd = open(dmxdev, O_RDWR)) < 0){
		perror("open");
		return 1;
	}

	if (set_filter(dmxfd, pid, filter, mask) != 0)
		return 1;

	for (;;) {
		process_section(dmxfd);
	}

	close(dmxfd);
	return 0;
}
