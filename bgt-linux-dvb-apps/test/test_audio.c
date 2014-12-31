/*
 * test_audio.c - Test program for new API
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
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <ost/dmx.h>
#include <ost/frontend_old.h>
#include <ost/sec.h>
#include <ost/audio.h>
#include <sys/poll.h>

int audioStop(int fd)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_STOP,0)) < 0) {
		perror("AUDIO STOP: ");
		return -1;
	}

	return 0;
}

int audioPlay(int fd)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_PLAY)) < 0) {
		perror("AUDIO PLAY: ");
		return -1;
	}

	return 0;
}


int audioPause(int fd)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_PAUSE)) < 0) {
		perror("AUDIO PAUSE: ");
		return -1;
	}

	return 0;
}


int audioContinue(int fd)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_CONTINUE)) < 0) {
		perror("AUDIO CONTINUE: ");
		return -1;
	}

	return 0;
}

int audioSelectSource(int fd, audio_stream_source_t source)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_SELECT_SOURCE, source)) < 0) {
		perror("AUDIO SELECT SOURCE: ");
		return -1;
	}

	return 0;
}



int audioSetMute(int fd, boolean state)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_SET_MUTE, state)) < 0) {
		perror("AUDIO SET MUTE: ");
		return -1;
	}

	return 0;
}

int audioSetAVSync(int fd,boolean state)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_SET_AV_SYNC, state)) < 0) {
		perror("AUDIO SET AV SYNC: ");
		return -1;
	}

	return 0;
}

int audioSetBypassMode(int fd,boolean mode)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_SET_BYPASS_MODE, mode)) < 0) {
		printf("AUDIO SET BYPASS MODE not implemented?\n");
		return -1;
	}

	return 0;
}


int audioChannelSelect(int fd, audio_channel_select_t select)
{
	int ans;

	if ((ans = ioctl(fd,AUDIO_CHANNEL_SELECT, select)) < 0) {
		perror("AUDIO CHANNEL SELECT: ");
		return -1;
	}

	return 0;
}

int audioGetStatus(int fd)
{
	struct audio_status stat;
	int ans;

	if ((ans = ioctl(fd,AUDIO_GET_STATUS, &stat)) < 0) {
		perror("AUDIO GET STATUS: ");
		return -1;
	}

	printf("Audio Status:\n");
	printf("  Sync State          : %s\n",
	       (stat.AV_sync_state ? "SYNC" : "NO SYNC"));
	printf("  Mute State          : %s\n",
	       (stat.mute_state ? "muted" : "not muted"));
	printf("  Play State          : ");
	switch ((int)stat.play_state){
	case AUDIO_STOPPED:
		printf("STOPPED (%d)\n",stat.play_state);
		break;
	case AUDIO_PLAYING:
		printf("PLAYING (%d)\n",stat.play_state);
		break;
	case AUDIO_PAUSED:
		printf("PAUSED (%d)\n",stat.play_state);
		break;
	default:
		printf("unknown (%d)\n",stat.play_state);
		break;
	}

	printf("  Stream Source       : ");
	switch((int)stat.stream_source){
	case AUDIO_SOURCE_DEMUX:
		printf("DEMUX (%d)\n",stat.stream_source);
		break;
	case AUDIO_SOURCE_MEMORY:
		printf("MEMORY (%d)\n",stat.stream_source);
		break;
	default:
		printf("unknown (%d)\n",stat.stream_source);
		break;
	}

	printf("  Channel Select      : ");
	switch((int)stat.channel_select){
	case AUDIO_STEREO:
		printf("Stereo (%d)\n",stat.channel_select);
		break;
	case AUDIO_MONO_LEFT:
		printf("Mono left(%d)\n",stat.channel_select);
		break;
	case AUDIO_MONO_RIGHT:
		printf("Mono right (%d)\n",stat.channel_select);
		break;
	default:
		printf("unknown (%d)\n",stat.channel_select);
		break;
	}
	printf("  Bypass Mode         : %s\n",
	       (stat.bypass_mode ? "ON" : "OFF"));

	return 0;

}

#define BUFFY 100000
#define NFD   2
play_file_audio(int filefd, int fd)
{
	char buf[BUFFY];
	int count;
	int written;
	int ch;
	struct pollfd pfd[NFD];
	int stopped = 0;
	boolean mute = false;
	boolean sync = false;

	pfd[0].fd = STDIN_FILENO;
	pfd[0].events = POLLIN;

	pfd[1].fd = fd;
	pfd[1].events = POLLOUT;


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
						audioPause(fd);
						printf("playback paused\n");
						stopped = 1;
						break;

					case 's':
						audioStop(fd);
						printf("playback stopped\n");
						stopped = 1;
						break;

					case 'c':
						audioContinue(fd);
						printf("playback continued\n");
						stopped = 0;
						break;

					case 'p':
						audioPlay(fd);
						printf("playback started\n");
						stopped = 0;
						break;

					case 'm':
						if (mute==false)mute=true;
						else mute=false;
						audioSetMute(fd,mute);
						printf("mute %d\n",mute);
						break;

					case 'a':
						if (sync==false)sync=true;
						else sync=false;
						audioSetAVSync(fd,sync);
						printf("AV sync %d\n",sync);
						stopped = 0;
						break;

					case 'q':
						audioContinue(fd);
						exit(0);
						break;
					}
				}

			}
		}
	}

}


main(int argc, char **argv)
{
	int fd;
	int filefd;
	boolean mute = false;
	boolean sync = false;

	if (argc < 2) return -1;

	if ( (filefd = open(argv[1],O_RDONLY)) < 0){
		perror("File open:");
		return -1;
	}

	if ((fd = open("/dev/ost/audio",O_RDWR|O_NONBLOCK)) < 0){
		perror("AUDIO DEVICE: ");
		return -1;
	}



	audioSetMute(fd,mute);
	audioSetBypassMode(fd,false);
	//audioContinue(fd);
	audioSelectSource(fd,AUDIO_SOURCE_MEMORY);
	audioPlay(fd);
	//sleep(4);
	//audioPause(fd);
	//sleep(3);
	//audioContinue(fd);
	//sleep(3);
	//audioStop(fd);
	//audioChannelSelect(fd,AUDIO_STEREO);
	//audioSetAVSync(fd,sync);
	audioGetStatus(fd);

	play_file_audio(filefd,fd);

	close(fd);
	return 0;


}
