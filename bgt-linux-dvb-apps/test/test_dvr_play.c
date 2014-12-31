/*
 * test_dvr_play.c - Play TS file via dvr device.
 *
 * Copyright (C) 2000 Ralph  Metzler <ralph@convergence.de>
 *                  & Marcus Metzler <marcus@convergence.de>
 *                    for convergence integrated media GmbH
 * Copyright (C) 2003 Convergence GmbH
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

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/poll.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <linux/dvb/dmx.h>


#define BUFSIZE (512*188)

void play_file_dvr(int filefd, int dvrfd)
{
	char buf[BUFSIZE];
	int count, written, bytes, total = 0;

	while ((count = read(filefd, buf, BUFSIZE)) > 0) {
		total += count;
		fprintf(stderr, "read  %d (%d total)\n", count, total);
		written = 0;
		while (written < count) {
			bytes = write(dvrfd, buf + written, count - written);
			fprintf(stderr, "write %d\n", bytes);
			if (bytes < 0) {
				perror("write dvr");
				return;
			}
			else if (bytes == 0) {
				fprintf(stderr, "write dvr: 0 bytes !");
				return;
			}
			written += bytes;
		}
	}
}

void set_pid(int fd, int pid, int type)
{
	struct dmx_pes_filter_params pesFilterParams;

	fprintf(stderr, "set PID 0x%04x (%d)\n", pid, type);
	if (ioctl(fd, DMX_STOP) < 0)
		perror("DMX STOP:");

	if (ioctl(fd, DMX_SET_BUFFER_SIZE, 64*1024) < 0)
		perror("DMX SET BUFFER:");

	pesFilterParams.pid = pid;
	pesFilterParams.input = DMX_IN_DVR;
	pesFilterParams.output = DMX_OUT_DECODER;
	pesFilterParams.pes_type = type;
	pesFilterParams.flags = DMX_IMMEDIATE_START;
	if (ioctl(fd, DMX_SET_PES_FILTER, &pesFilterParams) < 0)
		perror("DMX SET FILTER");
}

int main(int argc, char **argv)
{
	char *dmxdev = "/dev/dvb/adapter0/demux0";
	char *dvrdev = "/dev/dvb/adapter0/dvr0";
	int vpid, apid;
	int filefd, dvrfd, vfd, afd;

	if (argc < 4) {
		fprintf(stderr, "usage: test_dvr_play TS-file video-PID audio-PID\n");
		return 1;
	}
	vpid = strtoul(argv[2], NULL, 0);
	apid = strtoul(argv[3], NULL, 0);

	filefd = open(argv[1], O_RDONLY);
	if (filefd == -1) {
		fprintf(stderr, "Failed to open '%s': %d %m\n", argv[1], errno);
		return 1;
	}

	fprintf(stderr, "Playing '%s', video PID 0x%04x, audio PID 0x%04x\n",
			argv[1], vpid, apid);

	if (getenv("DEMUX"))
		dmxdev = getenv("DEMUX");
	if (getenv("DVR"))
		dvrdev = getenv("DVR");

	if ((dvrfd = open(dvrdev, O_WRONLY)) == -1) {
		fprintf(stderr, "Failed to open '%s': %d %m\n", dvrdev, errno);
		return 1;
	}

	if ((vfd = open(dmxdev, O_WRONLY)) == -1) {
		fprintf(stderr, "Failed to open video '%s': %d %m\n", dmxdev, errno);
		return 1;
	}
	if ((afd = open(dmxdev, O_WRONLY)) == -1) {
		fprintf(stderr, "Failed to open audio '%s': %d %m\n", dmxdev, errno);
		return 1;
	}

	/* playback timing is controlled via A/V PTS, so we cannot start
	 * writing to the DVR device before the PIDs are set...
	 */
	set_pid(afd, apid, DMX_PES_AUDIO);
	set_pid(vfd, vpid, DMX_PES_VIDEO);

	play_file_dvr(filefd, dvrfd);

	close(dvrfd);
	close(afd);
	close(vfd);
	close(filefd);
	return 0;
}
