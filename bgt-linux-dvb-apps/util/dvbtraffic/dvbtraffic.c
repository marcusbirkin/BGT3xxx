/* This file is released into the public domain by its authors */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <string.h>
#include <limits.h>
#include <libdvbapi/dvbdemux.h>

#define BSIZE 188

static int pidt[0x2001];

static void usage(FILE *output)
{
	fprintf(output,
		"Usage: dvbtraffic [OPTION]...\n"
		"Options:\n"
		"	-a N	use dvb adapter N\n"
		"	-d N	use demux N\n"
		"	-h	display this help\n");
}

int main(int argc, char **argv)
{
	struct timeval startt;
	int adapter = 0, demux = 0;
	char *search = NULL;
	int fd, ffd, packets = 0;
	int opt;

	while ((opt = getopt(argc, argv, "a:d:hs:")) != -1) {
		switch (opt) {
		case 'a':
			adapter = atoi(optarg);
			break;
		case 'd':
			demux = atoi(optarg);
			break;
		case 'h':
			usage(stdout);
			exit(0);
		case 's':
			search = strdup(optarg);
			break;
		default:
			usage(stderr);
			exit(1);
		}
	}

	// open the DVR device
	fd = dvbdemux_open_dvr(adapter, demux, 1, 0);
	if (fd < 0) {
 		fprintf(stderr, "dvbtraffic: Could not open dvr device: %m\n");
		exit(1);
	}
	dvbdemux_set_buffer(fd, 1024 * 1024);

	ffd = dvbdemux_open_demux(adapter, demux, 0);
	if (ffd < 0) {
		fprintf(stderr, "dvbtraffic: Could not open demux device: %m\n");
		exit(1);
	}

	if (dvbdemux_set_pid_filter(ffd, -1, DVBDEMUX_INPUT_FRONTEND, DVBDEMUX_OUTPUT_DVR, 1)) {
		perror("dvbdemux_set_pid_filter");
		return -1;
	}

	gettimeofday(&startt, 0);

	while (1) {
		unsigned char buffer[BSIZE];
		int pid, ok;
		ssize_t r;

		if ((r = read(fd, buffer, BSIZE)) <= 0) {
			perror("read");
			break;
		}
		if (r != BSIZE) {
			fprintf(stderr, "dvbtraffic: only read %zd bytes\n", r);
			break;
		}
		if (buffer[0] != 0x47) {
			continue;
			printf("desync (%x)\n", buffer[0]);
			while (buffer[0] != 0x47)
				read(fd, buffer, 1);
			continue;
		}
		ok = 1;
		pid = ((((unsigned) buffer[1]) << 8) |
		       ((unsigned) buffer[2])) & 0x1FFF;

		if (search) {
			int i, sl = strlen(search);
			ok = 0;
			if (pid != 0x1fff) {
				for (i = 0; i < (188 - sl); ++i) {
					if (!memcmp(buffer + i, search, sl))
						ok = 1;
				}
			}
		}

		if (ok) {
			pidt[pid]++;
			pidt[0x2000]++;
		}

		packets++;

		if (!(packets & 0xFF)) {
			struct timeval now;
			int diff;
			gettimeofday(&now, 0);
			diff =
			    (now.tv_sec - startt.tv_sec) * 1000 +
			    (now.tv_usec - startt.tv_usec) / 1000;
			if (diff > 1000) {
				int _pid = 0;
				for (_pid = 0; _pid < 0x2001; _pid++) {
					if (pidt[_pid]) {
						printf("%04x %5d p/s %5d kb/s %5d kbit\n",
						     _pid,
						     pidt[_pid] * 1000 / diff,
						     pidt[_pid] * 1000 / diff * 188 / 1024,
						     pidt[_pid] * 8 * 1000 / diff * 188 / 1000);
					}
					pidt[_pid] = 0;
				}
				printf("-PID--FREQ-----BANDWIDTH-BANDWIDTH-\n");
				startt = now;
			}
		}
	}

	close(ffd);
	close(fd);
	return 0;
}
