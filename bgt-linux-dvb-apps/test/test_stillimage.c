/* display single iframes as stillimages
 * iframes can be created with the 'convert' tool from imagemagick
 * and mpeg2encode from ftp.mpeg.org, and must have a supported
 * size, e.g. 702x576:
 *   $ convert -sample 702x576\! test.jpg test.mpg
 *
 * or more advanced using netpbm and mpeg2enc (not mpeg2encode) :
 *   $ cat image.jpg | jpegtopnm | pnmscale -xsize=704 -ysize=576 |\
 *      ppmntsc --pal | ppmtoy4m  -F 25:1 -A 4:3 -S 420mpeg2 |\
 *      mpeg2enc -f 7 -T 90 -F 3 -np -a 2 -o "image.mpg"
 *
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
#include <linux/dvb/video.h>


static const char *usage_string = "\n\t"
	"usage: %s <still.mpg> [still.mpg ...]\n"
	"\n\t"
	"to use another videodev than the first, set the environment variable VIDEODEV\n\t"
	"e.g.[user@linux]$ export VIDEODEV=\"/dev/dvb/adapter1/video1\".\n\t"
	"to display the image <n> seconds, instead of 10, set the variable SSTIME\n\t"
	"e.g. [user@linux]$ export SSTIME=\"60\" to display the still for 1 minute.\n\t"
	"this options can be set in the same line as the %s command:\n\t"
	"e.g. $ SSTIME=25 VIDEODEV=/dev/dvb/adapter1/video1 %s ...\n";


int main (int argc, char **argv)
{
	int fd;
	int filefd;
	struct stat st;
	struct video_still_picture sp;
	char *videodev = "/dev/dvb/adapter0/video0";
	char *env_sstime;
	int i = 1;
	int tsec = 10;

	if (argc < 2) {
		fprintf (stderr, usage_string, argv[0],argv[0],argv[0]);
		return -1;
	}

	if (getenv ("VIDEODEV"))
		videodev = getenv("VIDEODEV");

	if (getenv ("SSTIME")) {
		env_sstime = getenv("SSTIME");
		tsec = atoi(env_sstime);
	}

	if ((fd = open(videodev, O_RDWR)) < 0) {
		perror(videodev);
		return -1;
	}

next_pic:
	printf("I-frame     : '%s'\n", argv[i]);
	if ((filefd = open(argv[i], O_RDONLY)) < 0) {
		perror(argv[i]);
		return -1;
	}

	fstat(filefd, &st);

	sp.iFrame = (char *) malloc (st.st_size);
	sp.size = st.st_size;
	printf("I-frame size: %d\n", sp.size);

	if (!sp.iFrame) {
		fprintf (stderr, "No memory for I-Frame\n");
		return -1;
	}

	printf ("read: %d bytes\n", (int) read(filefd, sp.iFrame, sp.size));
	close(filefd);

	if ((ioctl(fd, VIDEO_STILLPICTURE, &sp) < 0)) {
		perror("ioctl VIDEO_STILLPICTURE");
		return -1;
	}
	free(sp.iFrame);

	printf("Display image %d seconds ...\n",tsec);
	sleep(tsec);
	printf("Done.\n");
	if (argc > ++i)
		goto next_pic;

	return 0;
}
