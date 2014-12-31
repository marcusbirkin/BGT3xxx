/* szap -- simple zapping tool for the Linux DVB API
 *
 * szap operates on VDR (http://www.cadsoft.de/people/kls/vdr/index.htm)
 * satellite channel lists (e.g. from http://www.dxandy.de/cgi-bin/dvbchan.pl).
 * szap assumes you have a "Universal LNB" (i.e. with LOFs 9750/10600 MHz).
 *
 * Compilation: `gcc -Wall -I../../ost/include -O2 szap.c -o szap`
 *  or, if your DVB driver is in the kernel source tree:
 *              `gcc -Wall -DDVB_IN_KERNEL -O2 szap.c -o szap`
 *
 * Copyright (C) 2001 Johannes Stezenbach (js@convergence.de)
 * for convergence integrated media
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

#include <stdint.h>
#include <sys/time.h>

#include "../include/frontend.h"
#include "../include/dmx.h"
#include "../include/audio.h"
#include "../include/version.h"
#include "lnb.h"

#ifndef TRUE
#define TRUE (1==1)
#endif
#ifndef FALSE
#define FALSE (1==0)
#endif

/* location of channel list file */
#define CHANNEL_FILE "channels.conf"

/* one line of the VDR channel file has the following format:
 * ^name:frequency_MHz:polarization:sat_no:symbolrate:vpid:apid:?:service_id$
 */


#define FRONTENDDEVICE "/dev/dvb/adapter%d/frontend%d"
#define DEMUXDEVICE "/dev/dvb/adapter%d/demux%d"
#define AUDIODEVICE "/dev/dvb/adapter%d/audio%d"

static struct lnb_types_st lnb_type;

static int exit_after_tuning;
static int interactive;

static char *usage_str =
    "\nusage: szap -q\n"
    "         list known channels\n"
    "       szap [options] {-n channel-number|channel_name}\n"
    "         zap to channel via number or full name (case insensitive)\n"
    "     -a number : use given adapter (default 0)\n"
    "     -f number : use given frontend (default 0)\n"
    "     -d number : use given demux (default 0)\n"
    "     -c file   : read channels list from 'file'\n"
    "     -b        : enable Audio Bypass (default no)\n"
    "     -x        : exit after tuning\n"
    "     -r        : set up /dev/dvb/adapterX/dvr0 for TS recording\n"
    "     -l lnb-type (DVB-S Only) (use -l help to print types) or \n"
    "     -l low[,high[,switch]] in Mhz\n"
    "     -i        : run interactively, allowing you to type in channel names\n"
    "     -p        : add pat and pmt to TS recording (implies -r)\n"
    "                 or -n numbers for zapping\n"
    "	  -t	    : delivery system type DVB-S=0, DSS=1, DVB-S2=2\n";

static int set_demux(int dmxfd, int pid, int pes_type, int dvr)
{
	struct dmx_pes_filter_params pesfilter;

	if (pid < 0 || pid >= 0x1fff) /* ignore this pid to allow radio services */
		return TRUE;

	if (dvr) {
		int buffersize = 64 * 1024;
		if (ioctl(dmxfd, DMX_SET_BUFFER_SIZE, buffersize) == -1)
			perror("DMX_SET_BUFFER_SIZE failed");
	}

	pesfilter.pid = pid;
	pesfilter.input = DMX_IN_FRONTEND;
	pesfilter.output = dvr ? DMX_OUT_TS_TAP : DMX_OUT_DECODER;
	pesfilter.pes_type = pes_type;
	pesfilter.flags = DMX_IMMEDIATE_START;

	if (ioctl(dmxfd, DMX_SET_PES_FILTER, &pesfilter) == -1) {
		fprintf(stderr, "DMX_SET_PES_FILTER failed "
			"(PID = 0x%04x): %d %m\n", pid, errno);

		return FALSE;
	}

	return TRUE;
}

int get_pmt_pid(char *dmxdev, int sid)
{
	int patfd, count;
	int pmt_pid = 0;
	int patread = 0;
	int section_length;
	unsigned char buft[4096];
	unsigned char *buf = buft;
	struct dmx_sct_filter_params f;

	memset(&f, 0, sizeof(f));
	f.pid = 0;
	f.filter.filter[0] = 0x00;
	f.filter.mask[0] = 0xff;
	f.timeout = 0;
	f.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

	if ((patfd = open(dmxdev, O_RDWR)) < 0) {
		perror("openening pat demux failed");
		return -1;
	}

	if (ioctl(patfd, DMX_SET_FILTER, &f) == -1) {
		perror("ioctl DMX_SET_FILTER failed");
		close(patfd);
		return -1;
	}

	while (!patread) {
		if (((count = read(patfd, buf, sizeof(buft))) < 0) && errno == EOVERFLOW)
			count = read(patfd, buf, sizeof(buft));
		if (count < 0) {
			perror("read_sections: read error");
			close(patfd);
			return -1;
		}

		section_length = ((buf[1] & 0x0f) << 8) | buf[2];
		if (count != section_length + 3)
			continue;

		buf += 8;
		section_length -= 8;

		patread = 1; /* assumes one section contains the whole pat */
		while (section_length > 0) {
			int service_id = (buf[0] << 8) | buf[1];
			if (service_id == sid) {
				pmt_pid = ((buf[2] & 0x1f) << 8) | buf[3];
				section_length = 0;
			}
			buf += 4;
			section_length -= 4;
		}
	}
	close(patfd);
	return pmt_pid;
}

struct diseqc_cmd {
	struct dvb_diseqc_master_cmd cmd;
	uint32_t wait;
};

void diseqc_send_msg(int fd, fe_sec_voltage_t v, struct diseqc_cmd *cmd,
		     fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b)
{
	if (ioctl(fd, FE_SET_TONE, SEC_TONE_OFF) == -1)
		perror("FE_SET_TONE failed");
	if (ioctl(fd, FE_SET_VOLTAGE, v) == -1)
		perror("FE_SET_VOLTAGE failed");
		usleep(15 * 1000);
	if (ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &cmd->cmd) == -1)
		perror("FE_DISEQC_SEND_MASTER_CMD failed");
		usleep(cmd->wait * 1000);
		usleep(15 * 1000);
	if (ioctl(fd, FE_DISEQC_SEND_BURST, b) == -1)
		perror("FE_DISEQC_SEND_BURST failed");
		usleep(15 * 1000);
	if (ioctl(fd, FE_SET_TONE, t) == -1)
		perror("FE_SET_TONE failed");
}




/* digital satellite equipment control,
 * specification is available from http://www.eutelsat.com/
 */
static int diseqc(int secfd, int sat_no, int pol_vert, int hi_band)
{
	struct diseqc_cmd cmd =
		{ {{0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00}, 4}, 0 };

	/**
	 * param: high nibble: reset bits, low nibble set bits,
	 * bits are: option, position, polarizaion, band
	 */
	cmd.cmd.msg[3] =
		0xf0 | (((sat_no * 4) & 0x0f) | (hi_band ? 1 : 0) | (pol_vert ? 0 : 2));

	diseqc_send_msg(secfd, pol_vert ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18,
			&cmd, hi_band ? SEC_TONE_ON : SEC_TONE_OFF,
			(sat_no / 4) % 2 ? SEC_MINI_B : SEC_MINI_A);

	return TRUE;
}

#define DVBS	0
#define DSS	1
#define DVBS2	2

static int do_tune(int fefd, unsigned int ifreq, unsigned int sr, unsigned int delsys)
{
	/*	API Major=3, Minor=1	*/
	struct dvb_frontend_parameters tuneto;
	struct dvb_frontend_event ev;
	/*	API Major=3, Minor=2	*/
	struct dvbfe_params fe_params;

	/* discard stale QPSK events */
	while (1) {
		if (ioctl(fefd, FE_GET_EVENT, &ev) == -1)
		break;
	}

	if ((DVB_API_VERSION == 3) && (DVB_API_VERSION_MINOR == 3)) {
		printf("\n%s: API version=%d, delivery system = %d\n", __func__, DVB_API_VERSION_MINOR, delsys);

		fe_params.frequency = ifreq;
		fe_params.inversion = INVERSION_AUTO;
		
		switch (delsys) {
		case DVBS:
			fe_params.delsys.dvbs.symbol_rate = sr;
			fe_params.delsys.dvbs.fec = FEC_AUTO;
			printf("%s: Frequency = %d, Srate = %d\n",
				__func__, fe_params.frequency, fe_params.delsys.dvbs.symbol_rate);
			break;
		case DSS:
			fe_params.delsys.dss.symbol_rate = sr;
			fe_params.delsys.dss.fec = FEC_AUTO;
			printf("%s: Frequency = %d, Srate = %d\n",
				__func__, fe_params.frequency, fe_params.delsys.dss.symbol_rate);
			break;
		case DVBS2:
			fe_params.delsys.dvbs2.symbol_rate = sr;
			fe_params.delsys.dvbs2.fec = FEC_AUTO;
			printf("%s: Frequency = %d, Srate = %d\n",
				__func__, fe_params.frequency, fe_params.delsys.dvbs2.symbol_rate);
			break;
		default:
			return -EINVAL;
		}
		printf("%s: Frequency = %d, Srate = %d\n\n\n",
			__func__, fe_params.frequency, fe_params.delsys.dvbs.symbol_rate);

		if (ioctl(fefd, DVBFE_SET_PARAMS, &fe_params) == -1) {
			perror("DVBFE_SET_PARAMS failed");
			return FALSE;
		}

	} else if ((DVB_API_VERSION == 3) && (DVB_API_VERSION_MINOR == 1)){
		tuneto.frequency = ifreq;
		tuneto.inversion = INVERSION_AUTO;
		tuneto.u.qpsk.symbol_rate = sr;
		tuneto.u.qpsk.fec_inner = FEC_AUTO;
		if (ioctl(fefd, FE_SET_FRONTEND, &tuneto) == -1) {
			perror("FE_SET_FRONTEND failed");
			return FALSE;
		}
	}
	return TRUE;
}


static
int check_frontend (int fe_fd, int dvr)
{
	(void)dvr;
	fe_status_t status;
	uint16_t snr, signal;
	uint32_t ber, uncorrected_blocks;
	int timeout = 0;

	do {
		if (ioctl(fe_fd, FE_READ_STATUS, &status) == -1)
			perror("FE_READ_STATUS failed");
		/* some frontends might not support all these ioctls, thus we
		 * avoid printing errors
		 */
		if (ioctl(fe_fd, FE_READ_SIGNAL_STRENGTH, &signal) == -1)
			signal = -2;
		if (ioctl(fe_fd, FE_READ_SNR, &snr) == -1)
			snr = -2;
		if (ioctl(fe_fd, FE_READ_BER, &ber) == -1)
			ber = -2;
		if (ioctl(fe_fd, FE_READ_UNCORRECTED_BLOCKS, &uncorrected_blocks) == -1)
			uncorrected_blocks = -2;

		printf ("status %02x | signal %04x | snr %04x | ber %08x | unc %08x | ",
			status, signal, snr, ber, uncorrected_blocks);

		if (status & FE_HAS_LOCK)
			printf("FE_HAS_LOCK");
		printf("\n");

		if (exit_after_tuning && ((status & FE_HAS_LOCK) || (++timeout >= 10)))
			break;

		usleep(1000000);
	} while (1);

	return 0;
}


static
int zap_to(unsigned int adapter, unsigned int frontend, unsigned int demux,
	   unsigned int sat_no, unsigned int freq, unsigned int pol,
	   unsigned int sr, unsigned int vpid, unsigned int apid, int sid,
	   int dvr, int rec_psi, int bypass, unsigned int delsys)
{
	char fedev[128], dmxdev[128], auddev[128];
	static int fefd, dmxfda, dmxfdv, audiofd = -1, patfd, pmtfd;
	int pmtpid;
	uint32_t ifreq;
	int hiband, result;
	enum dvbfe_delsys delivery;
	
	switch (delsys) {
	case DVBS:
		printf("Delivery system=DVB-S\n");
		delivery = DVBFE_DELSYS_DVBS;	
		break;
	case DSS:
		printf("Delivery system=DSS\n");
		delivery = DVBFE_DELSYS_DSS;
		break;
	case DVBS2:
		printf("Delivery system=DVB-S2\n");
		delivery = DVBFE_DELSYS_DVBS2;
		break;
	default:
		printf("Unsupported delivery system\n");
		return -EINVAL;
	}

	if (!fefd) {
		snprintf(fedev, sizeof(fedev), FRONTENDDEVICE, adapter, frontend);
		snprintf(dmxdev, sizeof(dmxdev), DEMUXDEVICE, adapter, demux);
		snprintf(auddev, sizeof(auddev), AUDIODEVICE, adapter, demux);
		printf("using '%s' and '%s'\n", fedev, dmxdev);

		if ((fefd = open(fedev, O_RDWR | O_NONBLOCK)) < 0) {
			perror("opening frontend failed");
			return FALSE;
		}
		result = ioctl(fefd, DVBFE_SET_DELSYS, &delivery);
		if (result < 0) {
			perror("ioctl DVBFE_SET_DELSYS failed");
			close(fefd);
			return FALSE;
		}

		if ((dmxfdv = open(dmxdev, O_RDWR)) < 0) {
			perror("opening video demux failed");
			close(fefd);
			return FALSE;
		}

		if ((dmxfda = open(dmxdev, O_RDWR)) < 0) {
			perror("opening audio demux failed");
			close(fefd);
			return FALSE;
		}

		if (dvr == 0)	/* DMX_OUT_DECODER */
			audiofd = open(auddev, O_RDWR);

		if (rec_psi){
			if ((patfd = open(dmxdev, O_RDWR)) < 0) {
				perror("opening pat demux failed");
				close(audiofd);
				close(dmxfda);
				close(dmxfdv);
				close(fefd);
				return FALSE;
			}

			if ((pmtfd = open(dmxdev, O_RDWR)) < 0) {
				perror("opening pmt demux failed");
				close(patfd);
				close(audiofd);
				close(dmxfda);
				close(dmxfdv);
				close(fefd);
				return FALSE;
			}
		}
	}

	hiband = 0;
	if (lnb_type.switch_val && lnb_type.high_val &&
		freq >= lnb_type.switch_val)
		hiband = 1;

	if (hiband)
		ifreq = freq - lnb_type.high_val;
	else {
		if (freq < lnb_type.low_val)
			ifreq = lnb_type.low_val - freq;
	else
		ifreq = freq - lnb_type.low_val;
	}
	result = FALSE;

	if (diseqc(fefd, sat_no, pol, hiband))
		if (do_tune(fefd, ifreq, sr, delsys))
			if (set_demux(dmxfdv, vpid, DMX_PES_VIDEO, dvr))
				if (audiofd >= 0)
					(void)ioctl(audiofd, AUDIO_SET_BYPASS_MODE, bypass);
	if (set_demux(dmxfda, apid, DMX_PES_AUDIO, dvr)) {
		if (rec_psi) {
			pmtpid = get_pmt_pid(dmxdev, sid);
			if (pmtpid < 0) {
				result = FALSE;
			}
			if (pmtpid == 0) {
				fprintf(stderr,"couldn't find pmt-pid for sid %04x\n",sid);
				result = FALSE;
			}
			if (set_demux(patfd, 0, DMX_PES_OTHER, dvr))
				if (set_demux(pmtfd, pmtpid, DMX_PES_OTHER, dvr))
					result = TRUE;
		} else {
			result = TRUE;
		}
	}

	check_frontend (fefd, dvr);

	if (!interactive) {
		close(patfd);
		close(pmtfd);
		if (audiofd >= 0)
			close(audiofd);
		close(dmxfda);
		close(dmxfdv);
		close(fefd);
	}

	return result;
}


static int read_channels(const char *filename, int list_channels,
			 uint32_t chan_no, const char *chan_name,
			 unsigned int adapter, unsigned int frontend,
			 unsigned int demux, int dvr, int rec_psi,
			 int bypass, unsigned int delsys)
{
	FILE *cfp;
	char buf[4096];
	char inp[256];
	char *field, *tmp, *p;
	unsigned int line;
	unsigned int freq, pol, sat_no, sr, vpid, apid, sid;
	int ret;

again:
	line = 0;
	if (!(cfp = fopen(filename, "r"))) {
		fprintf(stderr, "error opening channel list '%s': %d %m\n",
			filename, errno);
		return FALSE;
	}

	if (interactive) {
		fprintf(stderr, "\n>>> ");
		if (!fgets(inp, sizeof(inp), stdin)) {
			printf("\n");
			return -1;
		}
		if (inp[0] == '-' && inp[1] == 'n') {
			chan_no = strtoul(inp+2, NULL, 0);
			chan_name = NULL;
			if (!chan_no) {
				fprintf(stderr, "bad channel number\n");
				goto again;
			}
		} else {
			p = strchr(inp, '\n');
			if (p)
			*p = '\0';
			chan_name = inp;
			chan_no = 0;
		}
	}

	while (!feof(cfp)) {
		if (fgets(buf, sizeof(buf), cfp)) {
			line++;

		if (chan_no && chan_no != line)
			continue;

		tmp = buf;
		field = strsep(&tmp, ":");

		if (!field)
			goto syntax_err;

		if (list_channels) {
			printf("%03u %s\n", line, field);
			continue;
		}

		if (chan_name && strcasecmp(chan_name, field) != 0)
			continue;

		printf("zapping to %d '%s':\n", line, field);

		if (!(field = strsep(&tmp, ":")))
			goto syntax_err;

		freq = strtoul(field, NULL, 0);

		if (!(field = strsep(&tmp, ":")))
			goto syntax_err;

		pol = (field[0] == 'h' ? 0 : 1);

		if (!(field = strsep(&tmp, ":")))
			goto syntax_err;

		sat_no = strtoul(field, NULL, 0);

		if (!(field = strsep(&tmp, ":")))
			goto syntax_err;

		sr = strtoul(field, NULL, 0) * 1000;

		if (!(field = strsep(&tmp, ":")))
			goto syntax_err;

		vpid = strtoul(field, NULL, 0);
		if (!vpid)
			vpid = 0x1fff;

		if (!(field = strsep(&tmp, ":")))
			goto syntax_err;

		p = strchr(field, ';');

		if (p) {
			*p = '\0';
			p++;
			if (bypass) {
				if (!p || !*p)
					goto syntax_err;
				field = p;
			}
		}

		apid = strtoul(field, NULL, 0);
		if (!apid)
			apid = 0x1fff;

		if (!(field = strsep(&tmp, ":")))
			goto syntax_err;

		sid = strtoul(field, NULL, 0);

		printf("sat %u, frequency = %u MHz %c, symbolrate %u, "
		       "vpid = 0x%04x, apid = 0x%04x sid = 0x%04x\n",
			sat_no, freq, pol ? 'V' : 'H', sr, vpid, apid, sid);

		fclose(cfp);

		ret = zap_to(adapter, frontend, demux, sat_no, freq * 1000,
			     pol, sr, vpid, apid, sid, dvr, rec_psi, bypass, delsys);
		if (interactive)
			goto again;

		if (ret)
			return TRUE;

		return FALSE;

syntax_err:
		fprintf(stderr, "syntax error in line %u: '%s'\n", line, buf);
	} else if (ferror(cfp)) {
		fprintf(stderr, "error reading channel list '%s': %d %m\n",
		filename, errno);
		fclose(cfp);
		return FALSE;
	} else
		break;
	}

	fclose(cfp);

	if (!list_channels) {
		fprintf(stderr, "channel not found\n");

	if (!interactive)
		return FALSE;
	}
	if (interactive)
		goto again;

	return TRUE;
}


void
bad_usage(char *pname, int prlnb)
{
	int i;
	struct lnb_types_st *lnbp;
	char **cp;

	if (!prlnb) {
		fprintf (stderr, usage_str, pname);
	} else {
		i = 0;
		fprintf(stderr, "-l <lnb-type> or -l low[,high[,switch]] in Mhz\nwhere <lnb-type> is:\n");
		while(NULL != (lnbp = lnb_enum(i))) {
			fprintf (stderr, "%s\n", lnbp->name);
			for (cp = lnbp->desc; *cp ; cp++) {
				fprintf (stderr, "   %s\n", *cp);
			}
			i++;
		}
	}
}

int main(int argc, char *argv[])
{
	const char *home;
	char chanfile[2 * PATH_MAX];
	int list_channels = 0;
	unsigned int chan_no = 0;
	const char *chan_name = NULL;
	unsigned int adapter = 0, frontend = 0, demux = 0, dvr = 0, rec_psi = 0, delsys = 0;
	int bypass = 0;
	int opt, copt = 0;

	lnb_type = *lnb_enum(0);
	while ((opt = getopt(argc, argv, "hqrpn:a:f:d:t:c:l:xib")) != -1) {
		switch (opt) {
		case '?':
		case 'h':
		default:
			bad_usage(argv[0], 0);
		case 'b':
			bypass = 1;
			break;
		case 'q':
			list_channels = 1;
			break;
		case 'r':
			dvr = 1;
			break;
		case 'n':
			chan_no = strtoul(optarg, NULL, 0);
			break;
		case 'a':
			adapter = strtoul(optarg, NULL, 0);
			break;
		case 'f':
			frontend = strtoul(optarg, NULL, 0);
			break;
		case 'p':
			rec_psi = 1;
			break;
		case 'd':
			demux = strtoul(optarg, NULL, 0);
			break;
		case 't':
			delsys = strtoul(optarg, NULL, 0);
			break;
		case 'c':
			copt = 1;
			strncpy(chanfile, optarg, sizeof(chanfile));
			break;
		case 'l':
			if (lnb_decode(optarg, &lnb_type) < 0) {
				bad_usage(argv[0], 1);
				return -1;
			}
			break;
		case 'x':
			exit_after_tuning = 1;
			break;
		case 'i':
			interactive = 1;
			exit_after_tuning = 1;
		}
	}
	lnb_type.low_val *= 1000;	/* convert to kiloherz */
	lnb_type.high_val *= 1000;	/* convert to kiloherz */
	lnb_type.switch_val *= 1000;	/* convert to kiloherz */
	if (optind < argc)
		chan_name = argv[optind];
	if (chan_name && chan_no) {
		bad_usage(argv[0], 0);
		return -1;
	}
	if (list_channels && (chan_name || chan_no)) {
		bad_usage(argv[0], 0);
		return -1;
	}
	if (!list_channels && !chan_name && !chan_no && !interactive) {
		bad_usage(argv[0], 0);
		return -1;
	}

	if (!copt) {
		if (!(home = getenv("HOME"))) {
			fprintf(stderr, "error: $HOME not set\n");
		return TRUE;
	}
	snprintf(chanfile, sizeof(chanfile),
		"%s/.szap/%i/%s", home, adapter, CHANNEL_FILE);
	if (access(chanfile, R_OK))
		snprintf(chanfile, sizeof(chanfile),
			 "%s/.szap/%s", home, CHANNEL_FILE);
	}

	printf("reading channels from file '%s'\n", chanfile);

	if (rec_psi)
		dvr=1;

	if (!read_channels(chanfile, list_channels, chan_no, chan_name,
	    adapter, frontend, demux, dvr, rec_psi, bypass, delsys))

		return TRUE;

	return FALSE;
}
