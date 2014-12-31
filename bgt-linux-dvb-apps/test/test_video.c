/*
 * test_video.c - Test program for new API
 *
 * Copyright (C) 2000 Ralph  Metzler <ralph@convergence.de>
 *                  & Marcus Metzler <marcus@convergence.de>
                      for convergence integrated media GmbH
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
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/video.h>
#include <sys/poll.h>

int videoStop(int fd)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_STOP,0)) < 0) {
		perror("VIDEO STOP: ");
		return -1;
	}

	return 0;
}

int videoPlay(int fd)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_PLAY)) < 0) {
		perror("VIDEO PLAY: ");
		return -1;
	}

	return 0;
}


int videoFreeze(int fd)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_FREEZE)) < 0) {
		perror("VIDEO FREEZE: ");
		return -1;
	}

	return 0;
}


int videoContinue(int fd)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_CONTINUE)) < 0) {
		perror("VIDEO CONTINUE: ");
		return -1;
	}

	return 0;
}

int videoSelectSource(int fd, video_stream_source_t source)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_SELECT_SOURCE, source)) < 0) {
		perror("VIDEO SELECT SOURCE: ");
		return -1;
	}

	return 0;
}



int videoSetBlank(int fd, int state)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_SET_BLANK, state)) < 0) {
		perror("VIDEO SET BLANK: ");
		return -1;
	}

	return 0;
}

int videoFastForward(int fd,int nframes)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_FAST_FORWARD, nframes)) < 0) {
		perror("VIDEO FAST FORWARD: ");
		return -1;
	}

	return 0;
}

int videoSlowMotion(int fd,int nframes)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_SLOWMOTION, nframes)) < 0) {
		perror("VIDEO SLOWMOTION: ");
		return -1;
	}

	return 0;
}

int videoGetStatus(int fd)
{
	struct video_status vstat;
	int ans;

	if ((ans = ioctl(fd,VIDEO_GET_STATUS, &vstat)) < 0) {
		perror("VIDEO GET STATUS: ");
		return -1;
	}

	printf("Video Status:\n");
	printf("  Blank State          : %s\n",
	       (vstat.video_blank ? "BLANK" : "STILL"));
	printf("  Play State           : ");
	switch ((int)vstat.play_state){
	case VIDEO_STOPPED:
		printf("STOPPED (%d)\n",vstat.play_state);
		break;
	case VIDEO_PLAYING:
		printf("PLAYING (%d)\n",vstat.play_state);
		break;
	case VIDEO_FREEZED:
		printf("FREEZED (%d)\n",vstat.play_state);
		break;
	default:
		printf("unknown (%d)\n",vstat.play_state);
		break;
	}

	printf("  Stream Source        : ");
	switch((int)vstat.stream_source){
	case VIDEO_SOURCE_DEMUX:
		printf("DEMUX (%d)\n",vstat.stream_source);
		break;
	case VIDEO_SOURCE_MEMORY:
		printf("MEMORY (%d)\n",vstat.stream_source);
		break;
	default:
		printf("unknown (%d)\n",vstat.stream_source);
		break;
	}

	printf("  Format (Aspect Ratio): ");
	switch((int)vstat.video_format){
	case VIDEO_FORMAT_4_3:
		printf("4:3 (%d)\n",vstat.video_format);
		break;
	case VIDEO_FORMAT_16_9:
		printf("16:9 (%d)\n",vstat.video_format);
		break;
	default:
		printf("unknown (%d)\n",vstat.video_format);
		break;
	}

	printf("  Display Format       : ");
	switch((int)vstat.display_format){
	case VIDEO_PAN_SCAN:
		printf("Pan&Scan (%d)\n",vstat.display_format);
		break;
	case VIDEO_LETTER_BOX:
		printf("Letterbox (%d)\n",vstat.display_format);
		break;
	case VIDEO_CENTER_CUT_OUT:
		printf("Center cutout (%d)\n",vstat.display_format);
		break;
	default:
		printf("unknown (%d)\n",vstat.display_format);
		break;
	}
	return 0;
}

int videoStillPicture(int fd, struct video_still_picture *sp)
{
	int ans;

	if ((ans = ioctl(fd,VIDEO_STILLPICTURE, sp)) < 0) {
		perror("VIDEO STILLPICTURE: ");
		return -1;
	}

	return 0;
}

#define BUFFY 32768
#define NFD   2
void play_file_video(int filefd, int fd)
{
	char buf[BUFFY];
	int count;
	int written;
	struct pollfd pfd[NFD];
//	int stopped = 0;

	pfd[0].fd = STDIN_FILENO;
	pfd[0].events = POLLIN;

	pfd[1].fd = fd;
	pfd[1].events = POLLOUT;

	videoSelectSource(fd,VIDEO_SOURCE_MEMORY);
	videoPlay(fd);


	count = read(filefd,buf,BUFFY);
	write(fd,buf,count);

	while ( (count = read(filefd,buf,BUFFY)) >= 0  ){
		written = 0;
		while(written < count){
			if (poll(pfd,NFD,1)){
				if (pfd[1].revents & POLLOUT){
					written += write(fd,buf+written,
							count-written);
				}
				if (pfd[0].revents & POLLIN){
					int c = getchar();
					switch(c){
					case 'z':
						videoFreeze(fd);
						printf("playback frozen\n");
//						stopped = 1;
						break;

					case 's':
						videoStop(fd);
						printf("playback stopped\n");
//						stopped = 1;
						break;

					case 'c':
						videoContinue(fd);
						printf("playback continued\n");
//						stopped = 0;
						break;

					case 'p':
						videoPlay(fd);
						printf("playback started\n");
//						stopped = 0;
						break;

					case 'f':
						videoFastForward(fd,0);
						printf("fastforward\n");
//						stopped = 0;
						break;

					case 'm':
						videoSlowMotion(fd,2);
						printf("slowmotion\n");
//						stopped = 0;
						break;

					case 'q':
						videoContinue(fd);
						exit(0);
						break;
					}
				}

			}
		}
	}
}

void load_iframe(int filefd, int fd)
{
	struct stat st;
	struct video_still_picture sp;

	fstat(filefd, &st);

	sp.iFrame = (char *) malloc(st.st_size);
	sp.size = st.st_size;
	printf("I-frame size: %d\n", sp.size);

	if (!sp.iFrame) {
		printf("No memory for I-Frame\n");
		return;
	}

	printf("read: %d bytes\n",read(filefd,sp.iFrame,sp.size));
	videoStillPicture(fd,&sp);

	sleep(3);
	videoPlay(fd);
}

int main(int argc, char **argv)
{
	int fd;
	int filefd;

	if (argc < 2) return -1;

	if ( (filefd = open(argv[1],O_RDONLY)) < 0){
		perror("File open:");
		return -1;
	}
	if ((fd = open("/dev/dvb/adapter0/video0",O_RDWR|O_NONBLOCK)) < 0){
		perror("VIDEO DEVICE: ");
		return -1;
	}




//	videoSetBlank(fd,false);
	//videoPlay(fd);
	//sleep(4);
	//videoFreeze(fd);
	//sleep(3);
	//videoContinue(fd);
	//sleep(3);
	//videoStop(fd);
	videoGetStatus(fd);


	//load_iframe(filefd, fd);
	play_file_video(filefd, fd);
	close(fd);
	close(filefd);
	return 0;


}
