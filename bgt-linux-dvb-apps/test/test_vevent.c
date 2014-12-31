/*
 * test_vevent.c - Test VIDEO_GET_EVENT and poll(9 for video events
 *
 * Copyright (C) 2003 convergence GmbH
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

#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/dvb/video.h>


int videoGetSize(int fd, char *arg)
{
	video_size_t size;

	if (arg)
		return -1;
	if (ioctl(fd, VIDEO_GET_SIZE, &size) == -1){
		perror("VIDEO_GET_SIZE");
		return 0;
	}

	printf("Video Size: %ux%u ", size.w, size.h);
	switch (size.aspect_ratio) {
	case VIDEO_FORMAT_4_3:
		printf("4:3 (%d)\n", size.aspect_ratio);
		break;
	case VIDEO_FORMAT_16_9:
		printf("16:9 (%d)\n", size.aspect_ratio);
		break;
	case VIDEO_FORMAT_221_1:
		printf("2.21:1 (%d)\n", size.aspect_ratio);
		break;
	default:
		printf("unknown aspect ratio (%d)\n", size.aspect_ratio);
		break;
	}
	return 0;
}

int main(void)
{
	int vfd, rc;
	char *videodev = "/dev/dvb/adapter0/video0";
	struct pollfd pfd[1];
	struct video_event event;

	if (getenv("VIDEO"))
		videodev = getenv("VIDEO");

	printf("using video device '%s'\n", videodev);

	if ((vfd = open(videodev, O_RDONLY | O_NONBLOCK)) < 0) {
		perror("open video device");
		return 1;
	}

	videoGetSize(vfd, NULL);

	pfd[0].fd = vfd;
	pfd[0].events = POLLPRI;

	for (;;) {
		rc = poll(pfd, 1, -1);
		if (rc == -1) {
			perror("poll");
			return -1;
		}
		printf("poll events: %#x\n", pfd[0].revents);
		if (pfd[0].revents & POLLPRI) {
			rc = ioctl(vfd, VIDEO_GET_EVENT, &event);
			if (rc == -1) {
				perror("VIDEO_GET_EVENT");
				return -1;
			}
			printf("video event %d\n", event.type);
			if (event.type == VIDEO_EVENT_SIZE_CHANGED) {
				printf("  VIDEO_EVENT_SIZE_CHANGED %ux%u ",
						event.u.size.w, event.u.size.h);
				switch (event.u.size.aspect_ratio) {
				case VIDEO_FORMAT_4_3:
					printf("4:3 (%d)\n", event.u.size.aspect_ratio);
					break;
				case VIDEO_FORMAT_16_9:
					printf("16:9 (%d)\n", event.u.size.aspect_ratio);
					break;
				case VIDEO_FORMAT_221_1:
					printf("2.21:1 (%d)\n", event.u.size.aspect_ratio);
					break;
				default:
					printf("unknown aspect ratio (%d)\n",
							event.u.size.aspect_ratio);
					break;
				}
			}
		}
	}

	close(vfd);
	return 0;
}
