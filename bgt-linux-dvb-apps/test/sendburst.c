#define USAGE \
"\n" \
"\nTest sending the burst mini command A/B on a SAT frontend." \
"\n" \
"\nusage: FRONTEND=/dev/dvb/adapterX/frontendX sendburst {a|b}" \
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

   if (argc != 2 || (strcmp(argv[1], "a") && strcmp(argv[1], "b"))) {
      fprintf (stderr, "usage: %s <a|b>\n" USAGE, argv[0]);
      return 1;
   }

   if (getenv("FRONTEND"))
	   fedev = getenv("FRONTEND");

   printf("set22k: using '%s'\n", fedev);

   if ((fd = open (fedev, O_RDWR)) < 0) {
      perror ("open");
      return 1;
   }

   ioctl (fd, FE_SET_TONE, SEC_TONE_OFF);

   usleep (30000);    /*  30ms according to DiSEqC spec  */

   if (strcmp(argv[1], "a") == 0)
      r = ioctl (fd, FE_DISEQC_SEND_BURST, SEC_MINI_A);
   else
      r = ioctl (fd, FE_DISEQC_SEND_BURST, SEC_MINI_B);

   if (r == -1)
      perror("ioctl FE_SET_TONE");

   close (fd);

   return 0;
}
