#define USAGE \
"\n" \
"\nTest switching the 22kHz tone signal on and off on a SAT frontend." \
"\n(Note: DiSEqC equipment ignores this after it has once seen a diseqc" \
"\n sequence; reload the driver or unplug/replug the SAT cable to reset.)" \
"\n" \
"\nusage: FRONTEND=/dev/dvb/adapterX/frontendX set22k {on|off}" \
"\n"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/dvb/frontend.h>


int main (int argc, char **argv)
{
   char *fedev = "/dev/dvb/adapter0/frontend0";
   int fd, r;

   if (argc != 2 || (strcmp(argv[1], "on") && strcmp(argv[1], "off"))) {
      fprintf (stderr, "usage: %s <on|off>\n" USAGE, argv[0]);
      return 1;
   }
   if (getenv("FRONTEND"))
	   fedev = getenv("FRONTEND");
   printf("set22k: using '%s'\n", fedev);

   if ((fd = open (fedev, O_RDWR)) < 0) {
      perror ("open");
      return 1;
   }

   if (strcmp(argv[1], "on") == 0)
      r = ioctl (fd, FE_SET_TONE, SEC_TONE_ON);
   else
      r = ioctl (fd, FE_SET_TONE, SEC_TONE_OFF);
   if (r == -1)
	   perror("ioctl FE_SET_TONE");

   close (fd);

   return 0;
}
