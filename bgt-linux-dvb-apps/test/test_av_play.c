/*
 * test_av_play.c - Test playing an MPEG A+V PES (e.g. VDR recordings) from a file.
 *
 * Copyright (C) 2000 Ralph  Metzler <ralph@convergence.de>
 *                  & Marcus Metzler <marcus@convergence.de>
 *                    for convergence integrated media GmbH
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
 * Thu Jun 24 09:18:44 CEST 2004
 *   Add scan_file_av() and copy_to_dvb() for AV
 *   filtering to be able to use AC3 audio streams
 *   Copyright (C) 2004 Werner Fink <werner@suse.de>
 */

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>
#include <linux/dvb/audio.h>
#include <sys/poll.h>

static char dolby;
static char audio;
static char black;
static char volset;

static int audioPlay(int fd)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_PLAY)) < 0) {
		perror("AUDIO PLAY: ");
		return -1;
	}

	return 0;
}

static int audioSelectSource(int fd, audio_stream_source_t source)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_SELECT_SOURCE, source)) < 0) {
		perror("AUDIO SELECT SOURCE: ");
		return -1;
	}

	return 0;
}

static int audioSetMute(int fd, int state)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_SET_MUTE, state)) < 0) {
		perror("AUDIO SET MUTE: ");
		return -1;
	}

	return 0;
}

static int audioSetAVSync(int fd, int state)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_SET_AV_SYNC, state)) < 0) {
		perror("AUDIO SET AV SYNC: ");
		return -1;
	}

	return 0;
}

static int audioSetVolume(int fd, int level)
{
	int ans;
	audio_mixer_t mix;

	mix.volume_left = mix.volume_right = level;
	if ((ans = ioctl(fd, AUDIO_SET_MIXER, &mix)) < 0) {
		perror("AUDIO SET VOLUME: ");
		return -1;
	}
	return 0;
}

static int audioStop(int fd)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_STOP,0)) < 0) {
		perror("AUDIO STOP: ");
		return -1;
	}
	return 0;
}

static int deviceClear(int afd, int vfd)
{
	int ans;

	if (vfd >= 0) {
		if ((ans = ioctl(vfd, VIDEO_CLEAR_BUFFER, 0)) < 0) {
			perror("VIDEO CLEAR BUFFER: ");
			return -1;
		}
	}
	if (afd >= 0) {
		if ((ans = ioctl(afd, AUDIO_CLEAR_BUFFER, 0)) < 0) {
			perror("AUDIO CLEAR BUFFER: ");
			return -1;
		}
	}

	return 0;
}

static int videoStop(int fd)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_STOP,0)) < 0) {
		perror("VIDEO STOP: ");
		return -1;
	}

	return 0;
}

static int videoPlay(int fd)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_PLAY)) < 0) {
		perror("VIDEO PLAY: ");
		return -1;
	}

	return 0;
}

static int videoFreeze(int fd)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_FREEZE)) < 0) {
		perror("VIDEO FREEZE: ");
		return -1;
	}

	return 0;
}

static int videoBlank(int fd, int state)
{
	int ans;

	if ((ans = ioctl(fd, VIDEO_SET_BLANK, state)) < 0) {
		perror("VIDEO BLANK: ");
		return -1;
	}
	return 0;
}

static int videoContinue(int fd)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_CONTINUE)) < 0) {
		perror("VIDEO CONTINUE: ");
		return -1;
	}

	return 0;
}

static int videoSelectSource(int fd, video_stream_source_t source)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_SELECT_SOURCE, source)) < 0) {
		perror("VIDEO SELECT SOURCE: ");
		return -1;
	}

	return 0;
}


static int videoFastForward(int fd,int nframes)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_FAST_FORWARD, nframes)) < 0) {
		perror("VIDEO FAST FORWARD: ");
		return -1;
	}

	return 0;
}

static int videoSlowMotion(int fd,int nframes)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_SLOWMOTION, nframes)) < 0) {
		perror("VIDEO SLOWMOTION: ");
		return -1;
	}

	return 0;
}

#define NFD   3
static void copy_to_dvb(int vfd, int afd, int cfd, const uint8_t* ptr, const unsigned short len)
{
	struct pollfd pfd[NFD];
	unsigned short pos = 0;
//	int stopped = 0;

	pfd[0].fd = STDIN_FILENO;
	pfd[0].events = POLLIN;

	pfd[1].fd = vfd;
	pfd[1].events = POLLOUT;

	pfd[2].fd = afd;
	pfd[2].events = POLLOUT;

	while (pos < len) {
		int ret;
		if ((ret = poll(pfd,NFD,1)) > 0) {
			if (pfd[1].revents & POLLOUT) {
				int cnt = write(cfd, ptr + pos, len - pos);
				if (cnt > 0)
					pos += cnt;
				else if (cnt < 0) {
					if (errno != EAGAIN && errno != EINTR) {
						perror("Write:");
						exit(-1);
					}
					if (errno == EAGAIN)
						usleep(1000);
					continue;
				}
			}
			if (pfd[0].revents & POLLIN) {
				int c = getchar();
				switch(c) {
				case 'z':
					if (audio && !black) {
						audioSetMute(afd, 1);
					} else {
						videoFreeze(vfd);
					}
					deviceClear(afd, -1);
					printf("playback frozen\n");
//					stopped = 1;
					break;

				case 's':
					if (audio) {
						audioStop(afd);
						deviceClear(afd, -1);
					} else {
						videoStop(vfd);
						deviceClear(afd, vfd);
					}
					printf("playback stopped\n");
//					stopped = 1;
					break;

				case 'c':
					if (audio && !black) {
						audioSetAVSync(afd, 0);
						deviceClear(afd, -1);
						audioSetMute(afd, 0);
					} else {
						audioSetAVSync(afd, 1);
						deviceClear(afd, vfd);
						videoContinue(vfd);
					}
					printf("playback continued\n");
//					stopped = 0;
					break;

				case 'p':
					if (audio) {
						deviceClear(afd, -1);
						audioSetAVSync(afd, 0);
						audioPlay(afd);
					} else {
						deviceClear(afd, vfd);
						audioSetAVSync(afd, 1);
						audioPlay(afd);
						videoPlay(vfd);
					}
					audioSetMute(afd, 0);
					printf("playback started\n");
//					stopped = 0;
					break;

				case 'f':
					audioSetAVSync(afd, 0);
					if (!audio) {
						audioSetMute(afd, 1);
						videoFastForward(vfd,0);
					}
					printf("fastforward\n");
//					stopped = 0;
					break;

				case 'm':
					audioSetAVSync(afd, 0);
					audioSetMute(afd, 1);
					printf("mute\n");
//					stopped = 0;
					break;

				case 'u':
					audioSetAVSync(afd, 1);
					audioSetMute(afd, 0);
					printf("unmute\n");
//					stopped = 0;
					break;

				case 'd':
					if (dolby)
						dolby = 0;
					else
						dolby++;
					break;

				case 'l':
					audioSetAVSync(afd, 0);
					if (!audio) {
						audioSetMute(afd, 1);
						videoSlowMotion(vfd,2);
					}
					printf("slowmotion\n");
//					stopped = 0;
					break;

				case 'q':
					videoContinue(vfd);
					exit(0);
					break;

				default:
					break;
				}
			}
		} else if (ret < 0) {
			if (errno != EAGAIN && errno != EINTR) {
				perror("Write:");
				exit(-1);
			}
			if (errno == EAGAIN)
				usleep(1000);
		}
	}
}

static unsigned char play[6] = {0x00, 0x00, 0x01, 0xff, 0xff, 0xff};
static unsigned char except[2];

static int scan_file_av(int vfd, int afd, const unsigned char *buf, int buflen)
{
	const unsigned char *const start = buf;
	const unsigned char *const   end = buf + buflen;

	static unsigned int magic = 0xffffffff;
	static unsigned short count, len;
	static int fdc = -1;
	int m;

	while (buf < end) {
		if (count < 6) {
			switch (count) {
			case 0:
				m = 0;
				while ((magic & 0xffffff00) != 0x00000100) {
					if (buf >= end) goto out;
					magic = (magic << 8) | *buf++;
					m++;
				}
				if (m > 4)
					printf("Broken Frame found\n");
				play[3] = (unsigned char)(magic & 0x000000ff);
				switch (play[3]) {
				case 0xE0 ... 0xEF:
					fdc = vfd;
					if (except[0] != play[3]) {
						if (except[0] == 0)
							except[0] = play[3];
						else
							fdc = -1;
					}
					if (audio)
						fdc = -1;
					break;
				case 0xC0 ... 0xDF:
					fdc = afd;
					if (dolby) {
						fdc = -1;
						break;
					}
					if (except[1] != play[3]) {
						if (except[1] == 0)
							except[1] = play[3];
						else
							fdc = -1;
					}
					if (!volset) {
						audioSetVolume(afd, 255);
						volset = 1;
					}
					break;
				case 0xBD:
					/*
					 * TODO: sub filter to through out e.g. ub pictures
					 * in Private Streams 1 _and_ get sub audio header
					 * to set an except(ion) audio stream.
					 * The later one requires some changes within the VDR
					 * remux part!  2004/07/01 Werner
					 */
					fdc = afd;
					if (!dolby) {
						fdc = -1;
						break;
					}
					if (volset) {
						audioSetVolume(afd, 0);
						volset = 0;
					}
					break;
				default:
					fdc = -1;
					break;
				}
				count = 4;
			case 4:
				if (buf >= end) goto out;
				len = ((*buf) << 8);
				play[4] = (*buf);
				buf++;
				count++;
			case 5:
				if (buf >= end) goto out;
				len |= (*buf);
				len += 6;
				play[5] = (*buf);
				buf++;
				count++;
				if (fdc != -1)
					copy_to_dvb(vfd, afd, fdc, &play[0], count);
			default:
				break;
			}
		}

		while (count < len) {
			int rest = end - buf;
			if (rest <= 0) goto out;

			if (rest + count > len)
				rest = len - count;

			if (fdc != -1)
				copy_to_dvb(vfd, afd, fdc, buf, rest);
			count += rest;
			buf   += rest;
		}

		/* Reset for next scan */
		magic = 0xffffffff;
		count = len = 0;
		fdc = -1;
		play[3] = 0xff;
	}
out:
	return buf - start;
}

#define BUFFY 32768
static void play_file_av(int filefd, int vfd, int afd)
{
	unsigned char buf[BUFFY];
	int count;

	audioSetMute(afd, 1);
	videoBlank(vfd, 1);
	if (audio && !black) {
		audioStop(afd);
		deviceClear(afd, -1);
		audioSetAVSync(afd, 0);
		audioSelectSource(afd, AUDIO_SOURCE_MEMORY);
		audioPlay(afd);
		videoBlank(vfd, 0);
	} else if (audio && black) {
		deviceClear(afd, vfd);
		videoBlank(vfd, 1);
		audioSetAVSync(afd, 0);
		audioSelectSource(afd, AUDIO_SOURCE_MEMORY);
		videoSelectSource(vfd, VIDEO_SOURCE_MEMORY);
		audioPlay(afd);
		videoPlay(vfd);
	} else {
		deviceClear(afd, vfd);
		audioSetAVSync(afd, 1);
		audioSelectSource(afd, AUDIO_SOURCE_MEMORY);
		videoSelectSource(vfd, VIDEO_SOURCE_MEMORY);
		audioPlay(afd);
		videoPlay(vfd);
		videoBlank(vfd, 0);
	}

	if (dolby) {
		audioSetVolume(afd, 0);
		volset = 0;
	} else {
		audioSetVolume(afd, 255);
		volset = 1;
	}

#ifndef __stub_posix_fadvise
	posix_fadvise(filefd, 0, 0, POSIX_FADV_SEQUENTIAL);
#endif

	while ((count = read(filefd,buf,BUFFY)) > 0)
		scan_file_av(vfd,afd,buf,count);
}

static struct termios term;

static void restore(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

int main(int argc, char **argv)
{
	int vfd, afd, c;
	int filefd;
	const char *videodev = "/dev/dvb/adapter0/video0";
	const char *audiodev = "/dev/dvb/adapter0/audio0";

	if (((tcgetpgrp(STDIN_FILENO) == getpid()) || (getppid() != (pid_t)1))
	    && (tcgetattr(STDIN_FILENO, &term) == 0)) {
		struct termios newterm;
		memcpy(&newterm, &term, sizeof(struct termios));
		newterm.c_iflag = 0;
		newterm.c_lflag &= ~(ICANON | ECHO);
		newterm.c_cc[VMIN] = 0;
		newterm.c_cc[VTIME] = 0;
		atexit(restore);
		tcsetattr(STDIN_FILENO, TCSANOW, &newterm);
	}

	opterr = 0;
	while ((c = getopt(argc, argv, "+daA")) != -1) {
		switch (c) {
		case 'd':
			dolby++;
			break;
		case 'a':
			audio++;
			break;
		case 'A':
			audio++;
			black++;
			break;
		case '?':
			fprintf(stderr, "usage: test_av_play [-d] [-a] [-A] mpeg_A+V_PES_file\n");
			return 1;
		default:
			break;
		}
	}
	argv += optind;
	argc -= optind;

	if (getenv("VIDEO"))
		videodev = getenv("VIDEO");
	if (getenv("AUDIO"))
		audiodev = getenv("AUDIO");

	printf("using video device '%s'\n", videodev);
	printf("using audio device '%s'\n", audiodev);

	putchar('\n');

	printf("Freeze       by pressing `z'\n");
	printf("Stop         by pressing `s'\n");
	printf("Continue     by pressing `c'\n");
	printf("Start        by pressing `p'\n");
	printf("FastForward  by pressing `f'\n");
	printf("Mute         by pressing `m'\n");
	printf("UnMute       by pressing `u'\n");
	printf("MP2/AC3      by pressing `d'\n");
	printf("SlowMotion   by pressing `l'\n");
	printf("Quit         by pressing `q'\n");

	putchar('\n');

	errno = ENOENT;
	if (!argv[0] || (filefd = open(argv[0], O_RDONLY)) < 0) {
		perror("File open:");
		return -1;
	}
	if ((vfd = open(videodev,O_RDWR|O_NONBLOCK)) < 0) {
		perror("VIDEO DEVICE: ");
		return -1;
	}
	if ((afd = open(audiodev,O_RDWR|O_NONBLOCK)) < 0) {
		perror("AUDIO DEVICE: ");
		return -1;
	}

	play_file_av(filefd, vfd, afd);
	close(vfd);
	close(afd);
	close(filefd);
	return 0;
}
