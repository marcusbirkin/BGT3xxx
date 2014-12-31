/*
 * Tool for watching the dib3000*-demodulators,
 * with an extended output.
 *
 * Copyright (C) 2005 by Patrick Boettcher <patrick.boettcher@desy.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <getopt.h>

#include <signal.h>

#include <math.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/types.h>

#include "dib-i2c.h"
#include "dib3000-watch.h"
#include "dib3000.h"

void usage (void)
{
	verb("usage: dib3000-watch -d <i2c-device> -a <i2c-address> [-o <type>] [-i <seconds>]\n"
		 "   -d    normally one of /dev/i2c-[0-255]\n"
		 "   -a    is 8 for DiB3000M-B and 9, 10, 11 or 12 for DiB3000M-C or DiB3000-P\n"
		 "   -o    output type (print|csv) (default: print)\n"
		 "   -i    query interval in seconds (default: 0.1)\n"
		 "\n"
		 "Don't forget to run tzap or any other dvb-tune program (vdr, kaxtv) in order to tune a channel,\n"
		 "tuning isn't done by this tool.\n"
		 "\n"
		 "A lot of thing have been taken for the dibusb, dib3000m[bc] driver from kernel and\n"
		 "from t_demod-test software created by DiBcom. Both is GPL, so is dib-demod-watch.\n"
		 "\n"
		 "Copyright (C) 2005 by Patrick Boettcher <patrick.boettcher@desy.de>\n"
		 "\n"
		 "The source of this tool is released under the GPL.\n"
	);
	exit(1);
}

__u16 dib_read_reg(struct dib_demod *dib,__u16 reg)
{
	int ret;
	__u8 wb[] = { ((reg >> 8) | 0x80) & 0xff, reg & 0xff };
	__u8 rb[2];
	struct i2c_msg msg[] = {
		{ .addr = dib->i2c_addr, .flags = 0,        .buf = wb, .len = 2 },
		{ .addr = dib->i2c_addr, .flags = I2C_M_RD, .buf = rb, .len = 2 },
	};
	struct i2c_rdwr_ioctl_data i2c_data = {
		.msgs  = msg,
		.nmsgs = 2,
	};

	if ((ret = ioctl(dib->fd,I2C_RDWR,&i2c_data)) != 2) {
		err("i2c_rdwr read failed. (%d)\n",ret);
		return 0;
	}
	return (rb[0] << 8)| rb[1];
};

int dib_write_reg(struct dib_demod *dib, __u16 reg, __u16 val)
{
	int ret;
	__u8 b[] = {
		(reg >> 8) & 0xff, reg & 0xff,
		(val >> 8) & 0xff, val & 0xff,
	};
	struct i2c_msg msg[] = {
		{ .addr = dib->i2c_addr, .flags = 0, .buf = b, .len = 4 }
	};
	struct i2c_rdwr_ioctl_data i2c_data = {
		.msgs  = msg,
		.nmsgs = 1,
	};

	if ((ret = ioctl(dib->fd,I2C_RDWR,&i2c_data)) != 1) {
		err("i2c_rdwr write failed. (%d)\n",ret);
		return -1;
	}
	return 0;
}

int dib3000mb_monitoring(struct dib_demod *dib,struct dib3000mb_monitoring *m)
{
	int dds_freq, p_dds_freq,
		n_agc_power = dib_read_reg(dib,DIB3000MB_REG_AGC_POWER),
		rf_power = dib_read_reg(dib,DIB3000MB_REG_RF_POWER),
		timing_offset;
	double ad_power_dB, minor_power;

	m->invspec = dib_read_reg(dib,DIB3000MB_REG_DDS_INV);
	m->nfft = dib_read_reg(dib,DIB3000MB_REG_TPS_FFT);

	m->agc_lock = dib_read_reg(dib,DIB3000MB_REG_AGC_LOCK);
	m->carrier_lock = dib_read_reg(dib,DIB3000MB_REG_CARRIER_LOCK);
	m->tps_lock = dib_read_reg(dib,DIB3000MB_REG_TPS_LOCK);
	m->vit_lock = dib_read_reg(dib,DIB3000MB_REG_VIT_LCK);
	m->ts_sync_lock = dib_read_reg(dib,DIB3000MB_REG_TS_SYNC_LOCK);
	m->ts_data_lock = dib_read_reg(dib,DIB3000MB_REG_TS_RS_LOCK);

	p_dds_freq = ((dib_read_reg(dib,DIB3000MB_REG_DDS_FREQ_MSB) & 0xff) << 8) |
				 ((dib_read_reg(dib,DIB3000MB_REG_DDS_FREQ_LSB) & 0xff00) >> 8);
	dds_freq =   ((dib_read_reg(dib,DIB3000MB_REG_DDS_VALUE_MSB) & 0xff) << 8) |
				 ((dib_read_reg(dib,DIB3000MB_REG_DDS_VALUE_LSB) & 0xff00) >> 8);
	if (m->invspec)
		dds_freq = (1 << 16) - dds_freq;
	m->carrier_offset = (double)(dds_freq - p_dds_freq) / (double)(1 << 16) * DEF_SampFreq_KHz;

	m->ber = (double)((dib_read_reg(dib,DIB3000MB_REG_BER_MSB) << 16) | dib_read_reg(dib,DIB3000MB_REG_BER_LSB)) / (double) 1e8;
	m->per = dib_read_reg(dib,DIB3000MB_REG_PACKET_ERROR_RATE);
	m->unc = dib_read_reg(dib,DIB3000MB_REG_UNC);
	m->fft_pos = dib_read_reg(dib,DIB3000MB_REG_FFT_WINDOW_POS);
	m->snr = 10.0 * log10( (double)(dib_read_reg(dib,DIB3000MB_REG_SIGNAL_POWER) << 8) /
		(double)((dib_read_reg(dib,DIB3000MB_REG_NOISE_POWER_MSB) << 16) + dib_read_reg(dib,DIB3000MB_REG_NOISE_POWER_LSB)));

	m->mer = (double) ((dib_read_reg(dib,DIB3000MB_REG_MER_MSB) << 16) + dib_read_reg(dib,DIB3000MB_REG_MER_LSB))
		/ (double) (1<<9) / (m->nfft ? 767.0 : 191.0);

	if (n_agc_power == 0)
		n_agc_power = 1;
	ad_power_dB = 10 * log10( (double)(n_agc_power) / (double)(1<<16));
	minor_power = ad_power_dB - DEF_agc_ref_dB ;
	m->rf_power = -DEF_gain_slope_dB * (double)rf_power/(double)(1<<16) + DEF_gain_delta_dB + minor_power;

	timing_offset =
		(dib_read_reg(dib,DIB3000MB_REG_TIMING_OFFSET_MSB) << 16) + dib_read_reg(dib,DIB3000MB_REG_TIMING_OFFSET_LSB);
	if (timing_offset >= 0x800000)
		timing_offset |= 0xff000000;
	m->timing_offset_ppm = -(double)timing_offset / (double)(m->nfft ? 8192 : 2048) * 1e6 / (double)(1<<20);

	return 0;
}

int dib3000mb_print_monitoring(struct dib3000mb_monitoring *m)
{
	printf("DiB3000M-B status\n\n");
	printf(" AGC lock:                 %10d\n",m->agc_lock);
	printf(" carrier lock:             %10d\n",m->carrier_lock);
	printf(" TPS synchronize lock:     %10d\n",m->tps_lock);
	printf(" Viterbi lock:             %10d\n",m->vit_lock);
	printf(" MPEG TS synchronize lock: %10d\n",m->ts_sync_lock);
	printf(" MPEG TS data lock:        %10d\n",m->ts_data_lock);
	printf("\n\n");
	printf(" spectrum inversion:       %10d\n",m->invspec);
	printf(" carrier offset:           %3.7g\n",m->carrier_offset);
	printf("\n\n");
	printf(" bit error rate:           %3.7g\n",m->ber);
	printf(" packet error rate:        %10d\n",m->per);
	printf(" packet error count:       %10d\n",m->unc);
	printf("\n\n");
	printf(" fft position:             %10d\n",m->fft_pos);
	printf(" transmission mode:        %10s\n",m->nfft ? "8k" : "2k");
	printf("\n\n");
	printf(" C / (N + I) =             %3.7g\n",m->snr);
	printf(" MER  =                    %3.7g dB\n",m->mer);
	printf(" RF power =                %3.7g dBm\n",m->rf_power);
	printf(" timing offset =           %3.7g ppm\n",m->timing_offset_ppm);
	return 0;
}

int interrupted;

void sighandler (int sig)
{
	(void)sig;
	interrupted = 1;
}

typedef enum {
	OUT_PRINT = 0,
	OUT_CSV,
} dib3000m_output_t;

int main (int argc, char * const argv[])
{
	struct dib_demod dib;
	struct dib3000mb_monitoring mon;
	const char *dev = NULL;
	float intervall = 0.1;
	dib3000m_output_t out = OUT_PRINT;
	int c;

	while ((c = getopt(argc,argv,"d:a:o:i:")) != -1) {
		switch (c) {
			case 'd':
				dev = optarg;
				break;
			case 'a':
				dib.i2c_addr = atoi(optarg); /* The I2C address */
				break;
			case 'o':
				     if (strcasecmp(optarg,"print") == 0) out = OUT_PRINT;
				else if (strcasecmp(optarg,"csv") == 0)   out = OUT_CSV;
				else usage();
				break;
			case 'i':
				intervall = atof(optarg);
				break;
			default:
				usage();
		}
	}

	if (dev == NULL)
		usage();

	interrupted = 0;
	signal(SIGINT, sighandler);
	signal(SIGKILL, sighandler);
	signal(SIGHUP, sighandler);

	verb("will use '%s' as i2c-device and %d as i2c address.\n",dev,dib.i2c_addr);

	if ((dib.fd = open(dev,O_RDWR)) < 0) {
		err("could not open %s\n",dev);
		exit(1);
	}

    if (ioctl(dib.fd,I2C_SLAVE,dib.i2c_addr) < 0) {
		err("could not set i2c address\n");
		exit(1);
	}

	if (dib_read_reg(&dib,DIB3000_REG_MANUFACTOR_ID) != DIB3000_I2C_ID_DIBCOM) {
		err("could not find a dib3000 demodulator at i2c-address %d\n",dib.i2c_addr);
		exit(1);
	}

	switch (dib_read_reg(&dib,DIB3000_REG_DEVICE_ID)) {
		case DIB3000MB_DEVICE_ID:
			verb("found a DiB3000M-B demodulator.\n");
			dib.rev = DIB3000MB;
			break;
		case DIB3000MC_DEVICE_ID:
			verb("found a DiB3000M-C demodulator.\n");
			dib.rev = DIB3000MC;
			break;
		case DIB3000P_DEVICE_ID:
			verb("found a DiB3000-P demodulator.\n");
			dib.rev = DIB3000P;
			break;
		default:
			err("unsupported demodulator found.\n");
	}

	while (!interrupted) {
		switch (dib.rev) {
			case DIB3000MB:
				dib3000mb_monitoring(&dib,&mon);
				if (out == OUT_PRINT) {
					printf("\E[H\E[2J");
					dib3000mb_print_monitoring(&mon);
				} else if (out == OUT_CSV) {
					printf("no csv output implemented yet.\n");
				}
				break;
			default:
				interrupted=1;
				err("no monitoring writting for this demod, yet.\n");
		}
		usleep((int) (intervall * 1000000));
	}

	close(dib.fd);

	return 0;
}
