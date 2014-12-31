/*
 * lock_s - Ultra simple DVB-S lock test application
 * A minimal lock test application derived from szap.c
 * for testing purposes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/param.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#include <stdint.h>
#include <sys/time.h>

#include <linux/dvb/frontend.h>

#define FEDEVICE		"/dev/dvb/adapter%d/frontend%d"

static char *usage_str = "\n lock_s ver:0.0.1: Simple utility to test DVB-S signal lock.\n"
			 "     \n"
			 "     usage: lock_s [[PARAMS] [OPTIONS]]\n"
			 "       dvbstune [params] tune to user provided DVB-S parameters\n"
			 "     -a number : use given adapter (default 0)\n"
			 "     -f number : use given frontend (default 0)\n"
			 "     -p params : parameters to be used for tuning\n"
			 "	frequency (Mhz) Polarization (v/h) Symbol rate (MSPS)\n"
			 "      \n"
			 "     -s pos    : DiSEqC switch position\n"
			 "     -H        : human readable output\n"
			 "     -l lnb-type (DVB-S Only) (use -l help to print types) or \n"
			 "     -l low[,high[,switch]] in Mhz\n";

static char *univ_desc[] = {
	"Europe",
	"10800 to 11800 MHz and 11600 to 12700 Mhz",
	"Dual LO, loband 9750, hiband 10600 MHz",
	(char *)NULL
};

static char *dbs_desc[] = {
	"Expressvu, North America",
	"12200 to 12700 MHz",
	"Single LO, 11250 MHz",
	(char *)NULL
};

static char *standard_desc[] = {
	"10945 to 11450 Mhz",
	"Single LO, 10000 Mhz",
	(char *)NULL
};

static char *enhan_desc[] = {
	"Astra",
	"10700 to 11700 MHz",
	"Single LO, 9750 MHz",
	(char *)NULL
};

static char *cband_desc[] = {
	"Big Dish",
	"3700 to 4200 MHz",
	"Single LO, 5150 Mhz",
	(char *)NULL
};

enum sec_bands {
	SEC_LO_BAND	= 0,
	SEC_HI_BAND	= 1,
};

struct sec_params {
	unsigned int		freq;
	enum fe_sec_voltage	voltage;
	unsigned int		srate;

	unsigned int		freq_if;
	int			pos; /* DiSEqC sw. pos */

	fe_sec_tone_mode_t	tone;
	fe_sec_mini_cmd_t	burst;
};

struct lnb_types_st {
	char	*name;
	char	**desc;
	unsigned long	low_val;
	unsigned long	high_val;	/* zero indicates no hiband */
	unsigned long	switch_val;	/* zero indicates no hiband */
} lnbs[] = {
	{"UNIVERSAL",	univ_desc,	9750,  10600, 11700 },
 	{"DBS",		dbs_desc, 	11250, 0,     0     },
	{"STANDARD",	standard_desc,	10000, 0,     0     },
	{"ENHANCED",	enhan_desc,	9750,  0,     0     },
	{"C-BAND",	cband_desc,	5150,  0,     0     },
};

struct diseqc_cmd {
	struct dvb_diseqc_master_cmd cmd;
	uint32_t wait;
};

int lnb_parse(char *str, struct lnb_types_st *lnbp)
{
	int i;
	char *cp, *np;

	memset(lnbp, 0, sizeof (*lnbp));
	cp = str;
	while (*cp && isspace(*cp))
		cp++;

	if (isalpha(*cp)) {
		for (i = 0; i < (int)(sizeof(lnbs) / sizeof(lnbs[0])); i++) {
			if (!strcasecmp(lnbs[i].name, cp)) {
				*lnbp = lnbs[i];
				return 1;
			}
		}
		return -1;
	}

	if (*cp == '\0' || !isdigit(*cp))
		return -1;
	lnbp->low_val = strtoul(cp, &np, 0);
	if (lnbp->low_val == 0)
		return -1;

	cp = np;
	while(*cp && (isspace(*cp) || *cp == ','))
		cp++;
	if (*cp == '\0')
		return 1;
	if (!isdigit(*cp))
		return -1;
	lnbp->high_val = strtoul(cp, &np, 0);

	cp = np;
	while(*cp && (isspace(*cp) || *cp == ','))
		cp++;
	if (*cp == '\0')
		return 1;
	if (!isdigit(*cp))
		return -1;
	lnbp->switch_val = strtoul(cp, NULL, 0);
	return 1;
}

int params_decode(char *str, char **argv, struct sec_params *params)
{
	unsigned int freq;
	char *pol;
	char *cp, *np;

	cp = str;

	while (*cp && isspace(*cp))
		cp++;

	/* get frequency */
	if (*cp == '\0' || !isdigit(*cp))
		return -1;
	freq = strtoul(cp, &np, 0);
	params->freq = freq;
	if (freq == 0)
		return -1;

	/* polarization v=13v:0, h=18v:0 */
	pol = argv[optind]; /* v/h */
	(!strncmp(pol, "v", 1)) ? (params->voltage = 0) : (params->voltage = 1);
	params->srate = strtoul(argv[optind + 1], NULL, 0);

	return 1;
}

/**
 * lnb_setup(struct lnb_types_st *lnb, unsigned int freq, unsigned int *freq_if)
 * @lnb		: lnb type as described int lnb_types_st
 * @freq	: transponder frequency which user requests
 * @freq_if	: resultant Intermediate Frequency after down conversion
 */
static int lnb_setup(struct lnb_types_st *lnb, struct sec_params *params)
{
	int ret = 0;

	if (!lnb) {
		fprintf(stderr, "Error: lnb_types=NULL\n");
		ret = -EINVAL;
		goto err;
	}

	/* TODO! check upper and lower limits from LNB types */
	if (!params->freq) {
		fprintf(stderr, "Error: invalid frequency, f:%d", params->freq);
		ret = -EINVAL;
		goto err;
	}

	if (lnb->switch_val && lnb->high_val && params->freq >= lnb->switch_val) {
		/* HI Band */
		params->freq_if	= params->freq - lnb->high_val;
		params->tone	= SEC_TONE_ON;
	} else {
		/* LO Band */
		params->tone	= SEC_TONE_OFF;
		if (params->freq < lnb->low_val)
			params->freq_if = lnb->low_val - params->freq;
		else
			params->freq_if = params->freq - lnb->low_val;

	}
err:
	return ret;
}

static int diseqc_send_msg(int fd, struct sec_params *params, struct diseqc_cmd *cmd)
{
	int ret = 0;

	ret = ioctl(fd, FE_SET_TONE, SEC_TONE_OFF);
	if (ret < 0) {
		fprintf(stderr, "FE_SET_TONE failed\n");
		goto err;
	}

	ret = ioctl(fd, FE_SET_VOLTAGE, params->voltage);
	if (ret < 0) {
		fprintf(stderr, "FE_SET_VOLTAGE failed, voltage=%d\n", params->voltage);
		usleep(15 * 1000);
		goto err;
	}

	ret = ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &cmd->cmd);
	if (ret < 0) {
		perror("FE_DISEQC_SEND_MASTER_CMD failed");
		usleep(cmd->wait * 1000);
		usleep(15 * 1000);
		goto err;
	}

	ret = ioctl(fd, FE_DISEQC_SEND_BURST, params->burst);
	if (ret < 0) {
		fprintf(stderr, "FE_DISEQC_SEND_BURST failed, burst=%d\n", params->burst);
		usleep(15 * 1000);
		goto err;
	}

	ret = ioctl(fd, FE_SET_TONE, params->tone);
	if (ret < 0) {
		fprintf(stderr, "FE_SET_TONE failed, tone=%d\n", params->tone);
		goto err;
	}
err:
	return ret;
}

static int diseqc_setup(int fd, struct sec_params *params)
{
	int pos				= params->pos;
	int band			= params->tone;
	fe_sec_voltage_t voltage	= params->voltage;
	int ret;

	struct diseqc_cmd cmd = {{{0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00}, 4}, 0 };

	/*
	 * param: high nibble: reset bits, low nibble set bits,
	 * bits are: option, position, polarization, band
	 */
	cmd.cmd.msg[3] =	 0xf0			|
				(((pos * 4) & 0x0f)	|
				(band ? 1 : 0)		|
				(voltage ? 0 : 2));

	ret = diseqc_send_msg(fd, params, &cmd);
	if (ret < 0) {
		fprintf(stderr, "SEC message send failed, err=%d", ret);
		return -EIO;
	}
	return 0;
}

static int tune_to(int fd, struct sec_params *sec)
{
	struct dvb_frontend_parameters params;
	struct dvb_frontend_event ev;
	int ret;

	/* discard stale QPSK events */
	while (1) {
		ret = ioctl(fd, FE_GET_EVENT, &ev);
		if (ret == -1)
			break;
	}

	params.frequency		= sec->freq_if;
	params.inversion		= INVERSION_AUTO;
	params.u.qpsk.symbol_rate	= sec->srate;
	params.u.qpsk.fec_inner		= FEC_AUTO;

	ret = ioctl(fd, FE_SET_FRONTEND, &params);
	if (ret == -1) {
		fprintf(stderr, "FE_SET_FRONTEND error=%d\n", ret);
		return -1;
	}

	return 0;
}

static int frontend_open(int *fd, int adapter, int frontend)
{
	static struct dvb_frontend_info info;
	char fedev[128];
	int ret = 0;

	snprintf(fedev, sizeof(fedev), FEDEVICE, adapter, frontend);
	*fd = open(fedev, O_RDWR | O_NONBLOCK);
	if (*fd < 0) {
		fprintf(stderr, "Frontend %d open failed\n", frontend);
		ret = -1;
		goto err;
	}

	ret = ioctl(*fd, FE_GET_INFO, &info);
	if (ret < 0) {
		fprintf(stderr, "ioctl FE_GET_INFO failed\n");
		goto err;
	}

	if (info.type != FE_QPSK) {
		fprintf(stderr, "frontend device is not a QPSK (DVB-S) device!\n");
		ret = -ENODEV;
		goto err;
	}
	return 0;
err:
	fprintf(stderr, "Closing Adapter:%d Frontend:%d\n", adapter, frontend);
	close(*fd);
	return ret;
}

void bad_usage(char *pname, int prlnb)
{
	int i;
	struct lnb_types_st *lnb;
	char **cp;

	if (!prlnb) {
		fprintf (stderr, usage_str, pname);
	} else {
		i = 0;
		fprintf(stderr, "-l <lnb-type> or -l low[,high[,switch]] in Mhz\nwhere <lnb-type> is:\n");
		while ((lnb = &lnbs[i]) != NULL) {
			fprintf (stderr, "%s\n", lnb->name);
			for (cp = lnb->desc; *cp; cp++)
				fprintf (stderr, "   %s\n", *cp);
			i++;
		}
	}
}

static int check_frontend (int fd, int human_readable)
{
	fe_status_t status;
	uint16_t snr, signal;
	uint32_t ber, uncorrected_blocks;
	int ret;

	do {
		ret = ioctl(fd, FE_READ_STATUS, &status);
		if (ret == -1) {
			fprintf(stderr, "FE_READ_STATUS failed, err=%d", ret);
			return -EIO;
		}

		/*
		 * some frontends might not support all these ioctls, thus we
		 * avoid printing errors
		 */
		ret = ioctl(fd, FE_READ_SIGNAL_STRENGTH, &signal);
		if (ret == -1)
			signal = -2;

		ret = ioctl(fd, FE_READ_SNR, &snr);
		if (ret == -1)
			snr = -2;

		ret = ioctl(fd, FE_READ_BER, &ber);
		if (ret == -1)
			ber = -2;

		ret = ioctl(fd, FE_READ_UNCORRECTED_BLOCKS, &uncorrected_blocks);
		if (ret == -1)
			uncorrected_blocks = -2;

		if (human_readable) {
			printf ("status %02x | signal %3u%% | snr %3u%% | ber %d | unc %d | ",
				status, (signal * 100) / 0xffff, (snr * 100) / 0xffff, ber, uncorrected_blocks);
		} else {
			printf ("status %02x | signal %04x | snr %04x | ber %08x | unc %08x | ",
				status, signal, snr, ber, uncorrected_blocks);
		}

		if (status & FE_HAS_LOCK)
			printf("FE_HAS_LOCK");

		printf("\n");
		usleep(1000000);
	} while (1);

	return 0;
}


/**
 * basic options needed
 * @adapter
 * @frontend
 * @freq (Mhz)
 * @pol (13/18)
 * @srate (kSps)
 * @pos (0 - 3)
 * @LNB (low, high, switch) Mhz
 *
 */
int main(int argc, char *argv[])
{
	int opt;
	int ret, fd, adapter=0, frontend=0, pos;
//	int simple;
	struct lnb_types_st lnb;
	struct sec_params sec;

	lnb = lnbs[0]; /* default is Universal LNB */
	while ((opt = getopt(argc, argv, "Hh:a:f:p:s:l:")) != -1) {
		switch (opt) {
		case '?':
		case 'h':
		default: /* help */
			bad_usage(argv[0], 0);
			break;
		case 'a': /* adapter */
			adapter = strtoul(optarg, NULL, 0);
			break;
		case 'f': /* demodulator (device) */
			frontend = strtoul(optarg, NULL, 0);
			break;
		case 'p': /* parameters */
			if (params_decode(optarg, argv, &sec) < 0) {
				bad_usage(argv[0], 1);
				return -1;
			}
			break;
		case 's': /* diseqc position */
			pos = strtoul(optarg, NULL, 0);
			pos % 2 ? (sec.burst = SEC_MINI_B) : (sec.burst = SEC_MINI_A);
			break;
		case 'l':
			if (lnb_parse(optarg, &lnb) < 0) {
				bad_usage(argv[0], 1);
				return -1;
			}
			break;
		case 'H':
//			simple = 1; /* human readable */
			break;
		}
	}
	lnb.low_val	*= 1000; /* kHz */
	lnb.high_val	*= 1000; /* kHz */
	lnb.switch_val	*= 1000; /* kHz */

	sec.freq *= 1000; /* kHz */
	sec.srate *= 1000;

	ret = frontend_open(&fd, adapter, frontend);
	if (ret < 0) {
		fprintf(stderr, "Adapter:%d Frontend:%d open failed, err=%d\n", adapter, frontend, ret);
		return -1;
	}
	ret = lnb_setup(&lnb, &sec);
	if (ret < 0) {
		fprintf(stderr, "LNB setup failed, err=%d\n", ret);
		fprintf(stderr, "%s", usage_str);
		goto err;
	}

	ret = diseqc_setup(fd, &sec);
	if (ret < 0) {
		fprintf(stderr, "SEC setup failed, err=%d\n", ret);
		goto err;;
	}

	ret = tune_to(fd, &sec);
	if (ret < 0) {
		fprintf(stderr, "Adapter:%d Frontend:%d tune_to %d %d failed, err=%d\n", adapter, frontend, sec.freq, sec.srate, ret);
		goto err;;
	}

	ret = check_frontend(fd, 1);
	if (ret < 0) {
		fprintf(stderr, "check frontend failed\n");
		goto err;;
	}
err:
	return ret;
}
