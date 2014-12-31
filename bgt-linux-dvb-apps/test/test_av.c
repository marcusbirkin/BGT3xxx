/*
 * test_av.c - Test for audio and video MPEG decoder API.
 *
 * Copyright (C) 2000 - 2002 convergence GmbH
 * Ralph Metzler <rjkm@convergence.de>
 * Marcus Metzler <mocm@convergence.de>
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
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/video.h>

int audioStop(int fd, char *arg)
{
	if (arg)
		return -1;
	if (ioctl(fd, AUDIO_STOP) == -1)
		perror("AUDIO_STOP");
	return 0;
}

int audioPlay(int fd, char *arg)
{
	if (arg)
		return -1;
	if (ioctl(fd, AUDIO_PLAY)  == -1)
		perror("AUDIO_PLAY");
	return 0;
}

int audioPause(int fd, char *arg)
{
	if (arg)
		return -1;
	if (ioctl(fd, AUDIO_PAUSE) == -1)
		perror("AUDIO_PAUSE");
	return 0;
}


int audioContinue(int fd, char *arg)
{
	if (arg)
		return -1;
	if (ioctl(fd, AUDIO_CONTINUE) == -1)
		perror("AUDIO_CONTINUE");
	return 0;
}

int audioSelectSource(int fd, char *arg)
{
	int source;
	if (!arg)
		return -1;
	source = atoi(arg);
	if (ioctl(fd, AUDIO_SELECT_SOURCE, source) == -1)
		perror("AUDIO_SELECT_SOURCE");
	return 0;
}

int audioSetMute(int fd, char *arg)
{
	int mute;
	if (!arg)
		return -1;
	mute = atoi(arg);
	if (ioctl(fd, AUDIO_SET_MUTE, mute) == -1)
		perror("AUDIO_SET_MUTE");
	return 0;
}

int audioSetAVSync(int fd, char *arg)
{
	int _sync;
	if (!arg)
		return -1;
	_sync = atoi(arg);
	if (ioctl(fd, AUDIO_SET_AV_SYNC, _sync) == -1)
		perror("AUDIO_SET_AV_SYNC");
	return 0;
}

int audioSetBypassMode(int fd, char *arg)
{
	int byp;
	if (!arg)
		return -1;
	byp = atoi(arg);
	if (ioctl(fd, AUDIO_SET_BYPASS_MODE, byp) == -1)
		perror("AUDIO_SET_BYPASS_MODE");
	return 0;
}

int audioChannelSelect(int fd, char *arg)
{
	int chan;
	if (!arg)
		return -1;
	chan = atoi(arg);
	if (ioctl(fd, AUDIO_CHANNEL_SELECT, chan) == -1)
		perror("AUDIO_CHANNEL_SELECT");
	return 0;
}

int audioGetStatus(int fd, char *arg)
{
	struct audio_status _stat;

	if (arg)
		return -1;
	if (ioctl(fd, AUDIO_GET_STATUS, &_stat) == -1) {
		perror("AUDIO_GET_STATUS");
		return 0;
	}

	printf("Audio Status:\n");
	printf("  Sync State          : %s\n",
	       (_stat.AV_sync_state ? "SYNC" : "NO SYNC"));
	printf("  Mute State          : %s\n",
	       (_stat.mute_state ? "muted" : "not muted"));
	printf("  Play State          : ");
	switch ((int)_stat.play_state){
	case AUDIO_STOPPED:
		printf("STOPPED (%d)\n",_stat.play_state);
		break;
	case AUDIO_PLAYING:
		printf("PLAYING (%d)\n",_stat.play_state);
		break;
	case AUDIO_PAUSED:
		printf("PAUSED (%d)\n",_stat.play_state);
		break;
	default:
		printf("unknown (%d)\n",_stat.play_state);
		break;
	}

	printf("  Stream Source       : ");
	switch((int)_stat.stream_source){
	case AUDIO_SOURCE_DEMUX:
		printf("DEMUX (%d)\n",_stat.stream_source);
		break;
	case AUDIO_SOURCE_MEMORY:
		printf("MEMORY (%d)\n",_stat.stream_source);
		break;
	default:
		printf("unknown (%d)\n",_stat.stream_source);
		break;
	}

	printf("  Channel Select      : ");
	switch((int)_stat.channel_select){
	case AUDIO_STEREO:
		printf("Stereo (%d)\n",_stat.channel_select);
		break;
	case AUDIO_MONO_LEFT:
		printf("Mono left(%d)\n",_stat.channel_select);
		break;
	case AUDIO_MONO_RIGHT:
		printf("Mono right (%d)\n",_stat.channel_select);
		break;
	default:
		printf("unknown (%d)\n",_stat.channel_select);
		break;
	}
	printf("  Bypass Mode         : %s\n",
	       (_stat.bypass_mode ? "ON" : "OFF"));

	return 0;

}

int videoStop(int fd, char *arg)
{
	if (arg)
		return -1;
	if (ioctl(fd, VIDEO_STOP) == -1)
		perror("VIDEO_STOP");
	return 0;
}

int videoPlay(int fd, char *arg)
{
	if (arg)
		return -1;
	if (ioctl(fd, VIDEO_PLAY) == -1)
		perror("VIDEO_PLAY");
	return 0;
}


int videoFreeze(int fd, char *arg)
{
	if (arg)
		return -1;
	if (ioctl(fd, VIDEO_FREEZE) == -1)
		perror("VIDEO_FREEZE");
	return 0;
}


int videoContinue(int fd, char *arg)
{
	if (arg)
		return -1;
	if (ioctl(fd, VIDEO_CONTINUE) == -1)
		perror("VIDEO_CONTINUE");
	return 0;
}

int videoFormat(int fd, char *arg)
{
	int format;
	if (!arg)
		return -1;
	format = atoi(arg);
	if (ioctl(fd, VIDEO_SET_FORMAT, format) == -1)
		perror("VIDEO_SET_FORMAT");
	return 0;
}

int videoDisplayFormat(int fd, char *arg)
{
	int format;
	if (!arg)
		return -1;
	format = atoi(arg);
	if (ioctl(fd, VIDEO_SET_DISPLAY_FORMAT, format) == -1)
		perror("VIDEO_SET_DISPLAY_FORMAT");
	return 0;
}

int videoSelectSource(int fd, char *arg)
{
	int source;
	if (!arg)
		return -1;
	source = atoi(arg);
	if (ioctl(fd, VIDEO_SELECT_SOURCE, source) == -1)
		perror("VIDEO_SELECT_SOURCE");
	return 0;
}



int videoSetBlank(int fd, char *arg)
{
	int blank;
	if (!arg)
		return -1;
	blank = atoi(arg);
	if (ioctl(fd, VIDEO_SET_BLANK, blank) == -1)
		perror("VIDEO_SET_BLANK");
	return 0;
}

int videoFastForward(int fd, char *arg)
{
	int frames;
	if (!arg)
		return -1;
	frames = atoi(arg);
	if (ioctl(fd, VIDEO_FAST_FORWARD, frames) == -1)
		perror("VIDEO_FAST_FORWARD");
	return 0;
}

int videoSlowMotion(int fd, char *arg)
{
	int frames;
	if (!arg)
		return -1;
	frames = atoi(arg);
	if (ioctl(fd, VIDEO_SLOWMOTION, frames) == -1)
		perror("VIDEO_SLOWMOTION");
	return 0;
}

int videoGetStatus(int fd, char *arg)
{
	struct video_status _stat;

	if (arg)
		return -1;
	if (ioctl(fd, VIDEO_GET_STATUS, &_stat) == -1){
		perror("VIDEO_GET_STATUS");
		return 0;
	}

	printf("Video Status:\n");
	printf("  Blank State          : %s\n",
	       (_stat.video_blank ? "BLANK" : "STILL"));
	printf("  Play State           : ");
	switch ((int)_stat.play_state){
	case VIDEO_STOPPED:
		printf("STOPPED (%d)\n",_stat.play_state);
		break;
	case VIDEO_PLAYING:
		printf("PLAYING (%d)\n",_stat.play_state);
		break;
	case VIDEO_FREEZED:
		printf("FREEZED (%d)\n",_stat.play_state);
		break;
	default:
		printf("unknown (%d)\n",_stat.play_state);
		break;
	}

	printf("  Stream Source        : ");
	switch((int)_stat.stream_source){
	case VIDEO_SOURCE_DEMUX:
		printf("DEMUX (%d)\n",_stat.stream_source);
		break;
	case VIDEO_SOURCE_MEMORY:
		printf("MEMORY (%d)\n",_stat.stream_source);
		break;
	default:
		printf("unknown (%d)\n",_stat.stream_source);
		break;
	}

	printf("  Format (Aspect Ratio): ");
	switch((int)_stat.video_format){
	case VIDEO_FORMAT_4_3:
		printf("4:3 (%d)\n",_stat.video_format);
		break;
	case VIDEO_FORMAT_16_9:
		printf("16:9 (%d)\n",_stat.video_format);
		break;
	case VIDEO_FORMAT_221_1:
		printf("2.21:1 (%d)\n",_stat.video_format);
		break;
	default:
		printf("unknown (%d)\n",_stat.video_format);
		break;
	}

	printf("  Display Format       : ");
	switch((int)_stat.display_format){
	case VIDEO_PAN_SCAN:
		printf("Pan&Scan (%d)\n",_stat.display_format);
		break;
	case VIDEO_LETTER_BOX:
		printf("Letterbox (%d)\n",_stat.display_format);
		break;
	case VIDEO_CENTER_CUT_OUT:
		printf("Center cutout (%d)\n",_stat.display_format);
		break;
	default:
		printf("unknown (%d)\n",_stat.display_format);
		break;
	}
	return 0;
}

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

int videoStillPicture(int fd, char *arg)
{
	int sifd;
	struct stat st;
	struct video_still_picture sp;

	if (!arg)
		return -1;
	sifd = open(arg, O_RDONLY);
	if (sifd == -1) {
		perror("open stillimage file");
		printf("(%s)\n", arg);
		return 0;
	}
	fstat(sifd, &st);

	sp.iFrame = (char *) malloc(st.st_size);
	sp.size = st.st_size;
	printf("I-frame size: %d\n", sp.size);

	if (!sp.iFrame) {
		printf("No memory for I-Frame\n");
		return 0;
	}

	printf("read: %d bytes\n", (int) read(sifd,sp.iFrame,sp.size));
	if (ioctl(fd, VIDEO_STILLPICTURE, &sp) == -1)
		perror("VIDEO_STILLPICTURE");
	return 0;
}

typedef int (* cmd_func_t)(int fd, char *args);
typedef struct {
	char *cmd;
	char *param_help;
	cmd_func_t cmd_func;
} cmd_t;
cmd_t audio_cmds[] =
{
	{ "stop", "", audioStop },
	{ "play", "", audioPlay },
	{ "pause", "", audioPause },
	{ "continue", "", audioContinue },
	{ "source", "n: 0 demux, 1 memory", audioSelectSource },
	{ "mute", "n: 0 unmute, 1 mute", audioSetMute },
	{ "avsync", "n: 0 unsync, 1 sync", audioSetAVSync },
	{ "bypass", "n: 0 normal, 1 bypass", audioSetBypassMode },
	{ "channel", "n: 0 stereo, 1 left, 2 right", audioChannelSelect },
	{ "status", "", audioGetStatus },
	{ NULL, NULL, NULL }
};
cmd_t video_cmds[] =
{
	{ "stop", "", videoStop },
	{ "play", "", videoPlay },
	{ "freeze", "", videoFreeze },
	{ "continue", "", videoContinue },
	{ "source", "n: 0 demux, 1 memory", videoSelectSource },
	{ "blank", "n: 0 normal, 1 blank", videoSetBlank },
	{ "ff", "n: number of frames", videoFastForward },
	{ "slow", "n: number of frames", videoSlowMotion },
	{ "status", "", videoGetStatus },
	{ "size", "", videoGetSize },
	{ "stillpic", "filename", videoStillPicture},
	{ "format", "n: 0 4:3, 1 16:9", videoFormat},
	{ "dispformat", "n: 0 pan&scan, 1 letter box, 2 center cut out", videoDisplayFormat},
	{ NULL, NULL, NULL }
};
int usage(void)
{
	int i;
	printf ("commands begin with a for audio and v for video:\n\n"
	        "q : quit\n");
	for (i=0; audio_cmds[i].cmd; i++)
		printf("a %s %s\n", audio_cmds[i].cmd, audio_cmds[i].param_help);
	for (i=0; video_cmds[i].cmd; i++)
		printf("v %s %s\n", video_cmds[i].cmd, video_cmds[i].param_help);
	printf("\n");
	return 0;
}

int syntax_error(void)
{
	fprintf(stderr, "syntax error\n");
	return 0;
}

int process_kbd_input(int vfd, int afd)
{
	char buf[256], *cmd;
	int i;

	if (!fgets(buf, sizeof(buf), stdin))
		return -1;
	cmd = strtok(buf, " \n\t");
	if (!cmd) {
		printf("enter command or h + enter for help\n");
		return 0;
	}
	if (cmd[0] == 'q' || !strcmp(cmd, "quit"))
		return -1;
	if (cmd[0] == 'h' || !strcmp(cmd, "help"))
		return usage();
	if (cmd[0] == 'a' || !strcmp(cmd, "audio")) {
		cmd = strtok(NULL, " \n\t");
		if (!cmd)
			return syntax_error();
		for (i=0; audio_cmds[i].cmd; i++) {
			if (!strcmp(cmd, audio_cmds[i].cmd)) {
				cmd = strtok(NULL, " \n\t");
				printf("calling '%s', arg '%s'\n", audio_cmds[i].cmd, cmd);
				if (audio_cmds[i].cmd_func(afd, cmd))
					syntax_error();
				return 0;
			}
		}
		return syntax_error();
	} else if (buf[0] == 'v') {
		cmd = strtok(NULL, " \n\t");
		if (!cmd)
			return usage();
		for (i=0; video_cmds[i].cmd; i++) {
			if (!strcmp(cmd, video_cmds[i].cmd)) {
				cmd = strtok(NULL, " \n\t");
				printf("calling '%s', arg '%s'\n", video_cmds[i].cmd, cmd);
				if (video_cmds[i].cmd_func(vfd, cmd))
					syntax_error();
				return 0;
			}
		}
		return syntax_error();
	} else
		return syntax_error();
	return 0;
}

int main(void)
{
	int vfd, afd;
	char *videodev = "/dev/dvb/adapter0/video0";
	char *audiodev = "/dev/dvb/adapter0/audio0";

	if (getenv("VIDEO"))
		videodev = getenv("VIDEO");
	if (getenv("AUDIO"))
		videodev = getenv("AUDIO");

	printf("using video device '%s'\n", videodev);
	printf("using audio device '%s'\n", audiodev);
	printf("enter command or h + enter for help\n");

	if ((vfd = open(videodev, O_RDWR | O_NONBLOCK)) < 0) {
		perror("open video device");
		return 1;
	}
	if ((afd = open(audiodev, O_RDWR | O_NONBLOCK)) < 0) {
		perror("open audio device");
		return 1;
	}

	while (process_kbd_input(vfd, afd) == 0)
		;

	close(vfd);
	close(afd);
	return 0;
}
