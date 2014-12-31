#define USAGE \
"\n" \
"\nSet video and audio PIDs in the demux; useful only if you have" \
"\na hardware MPEG decoder and you're tuned to a transport stream." \
"\n" \
"\nusage: DEMUX=/dev/dvb/adapterX/demuxX setpid video_pid audio_pid" \
"\n"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <linux/dvb/dmx.h>


static
int setup_demux (char *dmxdev, int video_pid, int audio_pid)
{
   int vfd, afd;
   struct dmx_pes_filter_params pesfilter;

   printf ("video_pid == 0x%04x\n", video_pid);
   printf ("audio_pid == 0x%04x\n", audio_pid);

   if ((vfd = open (dmxdev, O_RDWR)) < 0) {
      perror("open 1");
      return -1;
   }

   pesfilter.pid = video_pid;
   pesfilter.input = DMX_IN_FRONTEND;
   pesfilter.output = DMX_OUT_DECODER;
   pesfilter.pes_type = DMX_PES_VIDEO;
   pesfilter.flags = DMX_IMMEDIATE_START;

   if (ioctl (vfd, DMX_SET_PES_FILTER, &pesfilter) < 0) {
      perror("ioctl DMX_SET_PES_FILTER (video)");
      return -1;
   }

   close (vfd);

   if ((afd = open (dmxdev, O_RDWR)) < 0) {
      perror("open 1");
      return -1;
   }

   pesfilter.pid = audio_pid;
   pesfilter.input = DMX_IN_FRONTEND;
   pesfilter.output = DMX_OUT_DECODER;
   pesfilter.pes_type = DMX_PES_AUDIO;
   pesfilter.flags = DMX_IMMEDIATE_START;

   if (ioctl (afd, DMX_SET_PES_FILTER, &pesfilter) < 0) {
      perror("ioctl DMX_SET_PES_FILTER (audio)");
      return -1;
   }

   close (afd);
   return 0;
}


int main (int argc, char **argv)
{
   char *dmxdev = "/dev/dvb/adapter0/demux0";
   int video_pid, audio_pid;

   if (argc != 3) {
      printf ("\nusage: %s <video pid> <audio pid>\n\n" USAGE, argv[0]);
      exit (1);
   }
   if (getenv("DEMUX"))
      dmxdev = getenv("DEMUX");
   printf("setpid: using '%s'\n", dmxdev);

   video_pid = strtol(argv[1], NULL, 0);
   audio_pid = strtol(argv[2], NULL, 0);
   if (setup_demux (dmxdev, video_pid, audio_pid) < 0)
      return 1;

   return 0;
}
