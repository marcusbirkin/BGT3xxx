#define USAGE \
"\n" \
"\nTest switching the voltage signal high and low on a SAT frontend." \
"\n(Note: DiSEqC equipment ignores this after it has once seen a diseqc" \
"\n sequence; reload the driver or unplug/replug the SAT cable to reset.)" \
"\n" \
"\nusage: FRONTEND=/dev/dvb/adapterX/frontendX setvoltage {13|18}" \
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

   if (argc != 2 || (strcmp(argv[1], "13") && strcmp(argv[1], "18"))) {
      fprintf (stderr, "usage: %s <13|18>\n" USAGE, argv[0]);
      return -1;
   }
   if (getenv("FRONTEND"))
	   fedev = getenv("FRONTEND");
   printf("setvoltage: using '%s'\n", fedev);

   if ((fd = open (fedev, O_RDWR)) < 0)
      perror ("open");

   if (strcmp(argv[1], "13") == 0)
      r = ioctl (fd, FE_SET_VOLTAGE, SEC_VOLTAGE_13);
   else
      r = ioctl (fd, FE_SET_VOLTAGE, SEC_VOLTAGE_18);
   if (r == -1)
	   perror ("ioctl FE_SET_VOLTAGE");

   close (fd);

   return 0;
}
