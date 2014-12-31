#define USAGE \
"\n" \
"\nTest sending DiSEqC commands on a SAT frontend." \
"\n" \
"\nusage: FRONTEND=/dev/dvb/adapterX/frontendX diseqc [test_seq_no|'all']" \
"\n"

#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/dvb/frontend.h>


struct diseqc_cmd {
	struct dvb_diseqc_master_cmd cmd;
	uint32_t wait;
};


struct diseqc_cmd switch_cmds[] = {
	{ { { 0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xf2, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xf1, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xf3, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xf4, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xf6, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xf5, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xf7, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xf8, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xfa, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xf9, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xfb, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xfc, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xfe, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xfd, 0x00, 0x00 }, 4 }, 0 },
	{ { { 0xe0, 0x10, 0x38, 0xff, 0x00, 0x00 }, 4 }, 0 }
};


/*--------------------------------------------------------------------------*/

static inline
void msleep(uint32_t msec)
{
	struct timespec req = { msec / 1000, 1000000 * (msec % 1000) };

	while (nanosleep(&req, &req))
		;
}


static
void diseqc_send_msg(int fd, fe_sec_voltage_t v, struct diseqc_cmd **cmd,
		     fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b)
{
	ioctl(fd, FE_SET_TONE, SEC_TONE_OFF);
	ioctl(fd, FE_SET_VOLTAGE, v);

	msleep(15);
	while (*cmd) {
		printf("msg: %02x %02x %02x %02x %02x %02x\n",
		       (*cmd)->cmd.msg[0], (*cmd)->cmd.msg[1],
		       (*cmd)->cmd.msg[2], (*cmd)->cmd.msg[3],
		       (*cmd)->cmd.msg[4], (*cmd)->cmd.msg[5]);

		ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &(*cmd)->cmd);
		msleep((*cmd)->wait);
		cmd++;
	}

	printf("%s: ", __FUNCTION__);

	printf(" %s ", v == SEC_VOLTAGE_13 ? "SEC_VOLTAGE_13" :
	       v == SEC_VOLTAGE_18 ? "SEC_VOLTAGE_18" : "???");

	printf(" %s ", b == SEC_MINI_A ? "SEC_MINI_A" :
	       b == SEC_MINI_B ? "SEC_MINI_B" : "???");

	printf(" %s\n", t == SEC_TONE_ON ? "SEC_TONE_ON" :
	       t == SEC_TONE_OFF ? "SEC_TONE_OFF" : "???");

	msleep(15);
	ioctl(fd, FE_DISEQC_SEND_BURST, b);

	msleep(15);
	ioctl(fd, FE_SET_TONE, t);
}


int main(int argc, char **argv)
{
	struct diseqc_cmd *cmd[2] = { NULL, NULL };
	char *fedev = "/dev/dvb/adapter0/frontend0";
	int fd;

	if (getenv("FRONTEND"))
		fedev = getenv("FRONTEND");

	printf("diseqc test: using '%s'\n", fedev);

	if ((fd = open(fedev, O_RDWR)) < 0) {
		perror("open");
		return -1;
	}

	if (argc != 2) {
	    fprintf (stderr, "usage: %s [number|'all']\n" USAGE, argv[0]);
	    return 1;
	} else if (strcmp(argv[1], "all")) {
		int i = atol(argv[1]);
		cmd[0] = &switch_cmds[i];
		diseqc_send_msg(fd,
				i % 2 ? SEC_VOLTAGE_18 : SEC_VOLTAGE_13,
				cmd,
				(i/2) % 2 ? SEC_TONE_ON : SEC_TONE_OFF,
				(i/4) % 2 ? SEC_MINI_B : SEC_MINI_A);
	} else {
		unsigned int j;

		for (j=0; j<sizeof(switch_cmds)/sizeof(struct diseqc_cmd); j++)
		{
			cmd[0] = &switch_cmds[j];
			diseqc_send_msg(fd,
					j % 2 ? SEC_VOLTAGE_18 : SEC_VOLTAGE_13,
					cmd,
					(j/2) % 2 ? SEC_TONE_ON : SEC_TONE_OFF,
					(j/4) % 2 ? SEC_MINI_B : SEC_MINI_A);
			msleep (1000);
		}
	}

	close(fd);

	return 0;
}
