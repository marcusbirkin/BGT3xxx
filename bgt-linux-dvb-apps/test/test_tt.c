/* test_tt.c - example of using PES filter for teletext output.
 *             c.f. ETSI EN-300-472
 *
 * usage: DEMUX=/dev/dvb/adapterX/demuxX test_tt PID
 *
 * Copyright (C) 2002 convergence GmbH
 * Johannes Stezenbach <js@convergence.de> based on code by
 * Ralph Metzler <ralph@convergence.de> and Marcus Metzler <marcus@convergence.de>
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <linux/dvb/dmx.h>

#include "hex_dump.h"

#define MAX_PES_SIZE (4*1024)


uint8_t reverse[] = {
0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF
};



void usage(void)
{
	fprintf(stderr, "usage: test_tt PID\n");
	fprintf(stderr, "       The default demux device used can be changed\n");
	fprintf(stderr, "       using the DEMUX environment variable\n");
	exit(1);
}

void safe_read(int fd, void *buf, int count)
{
	int bytes;

	do {
		bytes = read(fd, buf, count);
		if (bytes < 0) {
			if (errno == EOVERFLOW)
				fprintf(stderr, "read error: buffer overflow (%d)\n",
						EOVERFLOW);
			else {
				perror("read");
				exit(1);
			}
		}
		else if (bytes == 0) {
			fprintf(stderr, "got EOF on demux!?\n");
			exit(1);
		}
	} while (bytes < count);
}

void process_pes(int fd)
{
	uint8_t buf[MAX_PES_SIZE];
	int i, plen, hlen, sid, lines, l;

	/* search for start of next PES data block 0x000001bd */
	for (;;) {
		safe_read(fd, buf, 1);
		if (buf[0] != 0) continue;
		safe_read(fd, buf, 1);
		if (buf[0] != 0) continue;
		safe_read(fd, buf, 1);
		if (buf[0] != 1) continue;
		safe_read(fd, buf, 1);
		if (buf[0] == 0xbd)
			break;
	}

	safe_read(fd, buf, 5);
	/* PES packet length */
	plen = ((buf[0] << 8) | buf[1]) & 0xffff;

	/* PES header data length */
	hlen = buf[4];
	if (hlen != 0x24) {
		fprintf(stderr, "error: PES header data length != 0x24 (0x%02x)\n", hlen);
		return;
	}
	/* skip rest of PES header */
	safe_read(fd, buf, hlen);

	/* read stream ID */
	safe_read(fd, buf, 1);
	sid = buf[0];
	if (sid < 0x10 || sid > 0x1f) {
		fprintf(stderr, "error: non-EBU stream ID 0x%02x\n", sid);
		return;
	}

	/* number of VBI lines */
	lines = (plen + 6) / 46 - 1;

	/* read VBI data */
	for (l = 0; l < lines; l++) {
		safe_read(fd, buf, 46);
		if (buf[1] != 44) {
			fprintf(stderr, "error: VBI line has invalid length\n");
			return;
		}
		/* bit twiddling */
		for (i = 2; i < 46; i++)
			buf[i] = reverse[buf[i]];
		/* framing code, should be 11100100b */
		if (buf[3] != 0x27) {
			fprintf(stderr, "error: wrong framing code\n");
			return;
		}
		/* remaining data needs to be hamming decoded, but we should
		 * be able to read some partial strings */
		hex_dump(buf, 46);
		printf("\n");
	}
}

int set_filter(int fd, unsigned int pid)
{
	struct dmx_pes_filter_params f;

	f.pid = (uint16_t) pid;
	f.input = DMX_IN_FRONTEND;
	f.output = DMX_OUT_TAP;
	f.pes_type = DMX_PES_OTHER; /* DMX_PES_TELETEXT if you want vbi insertion */
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
	char * dmxdev = "/dev/dvb/adapter0/demux0";

	if (argc != 2)
		usage();

	pid = strtoul(argv[1], NULL, 0);
	if (pid > 0x1fff)
		usage();
	if (getenv("DEMUX"))
		dmxdev = getenv("DEMUX");
	fprintf(stderr, "test_tt: using '%s'\n", dmxdev);
	fprintf(stderr, "         PID 0x%04lx\n", pid);

	if ((dmxfd = open(dmxdev, O_RDWR)) < 0){
		perror("open");
		return 1;
	}

	if (set_filter(dmxfd, pid) != 0)
		return 1;

	for (;;) {
		process_pes(dmxfd);
	}

	close(dmxfd);
	return 0;
}
