/* test_stc.c - Test for DMX_GET_STC.
 * usage: DEMUX=/dev/dvb/adapterX/demuxX test_stc
 *
 * Copyright (C) 2003 convergence GmbH
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


void usage(void)
{
	fprintf(stderr, "usage: test_stc\n");
	fprintf(stderr, "       The default demux device used can be changed\n");
	fprintf(stderr, "       using the DEMUX environment variable;\n");
        fprintf(stderr, "\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	int dmxfd;
	char * dmxdev = "/dev/dvb/adapter0/demux0";
	struct dmx_stc stc;

	if (argc != 1 || argv[1])
		usage();

	if (getenv("DEMUX"))
		dmxdev = getenv("DEMUX");
	fprintf(stderr, "test_stc: using '%s'\n", dmxdev);

	if ((dmxfd = open(dmxdev, O_RDWR)) < 0) {
		perror("open");
		return 1;
	}

	stc.num = 0;
	if (ioctl(dmxfd, DMX_GET_STC, &stc) == -1) {
		perror("DMX_GET_STC");
		return 1;
	}

	printf("STC = %llu (%llu / %u)\n", stc.stc / stc.base, stc.stc, stc.base);

	close(dmxfd);
	return 0;
}
