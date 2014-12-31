/*
 * libdvbfe - a DVB frontend library
 *
 * Copyright (C) 2005 Andrew de Quincey (adq_dvb@lidskialf.net)
 * Copyright (C) 2005 Manu Abraham <abraham.manu@gmail.com>
 * Copyright (C) 2005 Kenneth Aafloy (kenneth@linuxtv.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <linux/dvb/frontend.h>
#include <libdvbmisc/dvbmisc.h>
#include "dvbfe.h"

int verbose = 0;

static int dvbfe_spectral_inversion_to_kapi[][2] =
{
	{ DVBFE_INVERSION_OFF, INVERSION_OFF },
	{ DVBFE_INVERSION_ON, INVERSION_ON },
	{ DVBFE_INVERSION_AUTO, INVERSION_AUTO },
	{ -1, -1 }
};

static int dvbfe_code_rate_to_kapi[][2] =
{
	{ DVBFE_FEC_NONE, FEC_NONE },
	{ DVBFE_FEC_1_2, FEC_1_2 },
	{ DVBFE_FEC_2_3, FEC_2_3 },
	{ DVBFE_FEC_3_4, FEC_3_4 },
	{ DVBFE_FEC_4_5, FEC_4_5 },
	{ DVBFE_FEC_5_6, FEC_5_6 },
	{ DVBFE_FEC_6_7, FEC_6_7 },
	{ DVBFE_FEC_7_8, FEC_7_8 },
	{ DVBFE_FEC_8_9, FEC_8_9 },
	{ DVBFE_FEC_AUTO, FEC_AUTO },
	{ -1, -1 }
};

static int dvbfe_dvbt_const_to_kapi[][2] =
{
	{ DVBFE_DVBT_CONST_QPSK, FE_QPSK },
	{ DVBFE_DVBT_CONST_QAM_16, QAM_16 },
	{ DVBFE_DVBT_CONST_QAM_32, QAM_32 },
	{ DVBFE_DVBT_CONST_QAM_64, QAM_64 },
	{ DVBFE_DVBT_CONST_QAM_128, QAM_128 },
	{ DVBFE_DVBT_CONST_QAM_256, QAM_256 },
	{ DVBFE_DVBT_CONST_AUTO, QAM_AUTO },
	{ -1, -1 }
};

static int dvbfe_dvbc_mod_to_kapi[][2] =
{
	{ DVBFE_DVBC_MOD_QAM_16, QAM_16 },
	{ DVBFE_DVBC_MOD_QAM_32, QAM_32 },
	{ DVBFE_DVBC_MOD_QAM_64, QAM_64 },
	{ DVBFE_DVBC_MOD_QAM_128, QAM_128 },
	{ DVBFE_DVBC_MOD_QAM_256, QAM_256 },
	{ DVBFE_DVBC_MOD_AUTO, QAM_AUTO },
	{ -1, -1 }
};

static int dvbfe_atsc_mod_to_kapi[][2] =
{
	{ DVBFE_ATSC_MOD_QAM_64, QAM_64 },
	{ DVBFE_ATSC_MOD_QAM_256, QAM_256 },
	{ DVBFE_ATSC_MOD_VSB_8, VSB_8 },
	{ DVBFE_ATSC_MOD_VSB_16, VSB_16 },
	{ DVBFE_ATSC_MOD_AUTO, QAM_AUTO },
	{ -1, -1 }
};

static int dvbfe_dvbt_transmit_mode_to_kapi[][2] =
{
	{ DVBFE_DVBT_TRANSMISSION_MODE_2K, TRANSMISSION_MODE_2K },
	{ DVBFE_DVBT_TRANSMISSION_MODE_8K, TRANSMISSION_MODE_8K },
	{ DVBFE_DVBT_TRANSMISSION_MODE_AUTO, TRANSMISSION_MODE_AUTO },
	{ -1, -1 }
};

static int dvbfe_dvbt_bandwidth_to_kapi[][2] =
{
	{ DVBFE_DVBT_BANDWIDTH_8_MHZ, BANDWIDTH_8_MHZ },
	{ DVBFE_DVBT_BANDWIDTH_7_MHZ, BANDWIDTH_7_MHZ },
	{ DVBFE_DVBT_BANDWIDTH_6_MHZ, BANDWIDTH_6_MHZ },
	{ DVBFE_DVBT_BANDWIDTH_AUTO, BANDWIDTH_AUTO },
	{ -1, -1 }
};

static int dvbfe_dvbt_guard_interval_to_kapi[][2] =
{
	{ DVBFE_DVBT_GUARD_INTERVAL_1_32, GUARD_INTERVAL_1_32},
	{ DVBFE_DVBT_GUARD_INTERVAL_1_16, GUARD_INTERVAL_1_16},
	{ DVBFE_DVBT_GUARD_INTERVAL_1_8, GUARD_INTERVAL_1_8},
	{ DVBFE_DVBT_GUARD_INTERVAL_1_4, GUARD_INTERVAL_1_4},
	{ DVBFE_DVBT_GUARD_INTERVAL_AUTO, GUARD_INTERVAL_AUTO},
	{ -1, -1 }
};

static int dvbfe_dvbt_hierarchy_to_kapi[][2] =
{
	{ DVBFE_DVBT_HIERARCHY_NONE, HIERARCHY_NONE },
	{ DVBFE_DVBT_HIERARCHY_1, HIERARCHY_1 },
	{ DVBFE_DVBT_HIERARCHY_2, HIERARCHY_2 },
	{ DVBFE_DVBT_HIERARCHY_4, HIERARCHY_4 },
	{ DVBFE_DVBT_HIERARCHY_AUTO, HIERARCHY_AUTO },
	{ -1, -1 }
};


static int lookupval(int val, int reverse, int table[][2])
{
	int i =0;

	while(table[i][0] != -1) {
		if (!reverse) {
			if (val == table[i][0]) {
				return table[i][1];
			}
		} else {
			if (val == table[i][1]) {
				return table[i][0];
			}
		}
		i++;
	}

	return -1;
}


struct dvbfe_handle {
	int fd;
	enum dvbfe_type type;
	char *name;
};

struct dvbfe_handle *dvbfe_open(int adapter, int frontend, int readonly)
{
	char filename[PATH_MAX+1];
	struct dvbfe_handle *fehandle;
	int fd;
	struct dvb_frontend_info info;

	//  flags
	int flags = O_RDWR;
	if (readonly) {
		flags = O_RDONLY;
	}

	// open it (try normal /dev structure first)
	sprintf(filename, "/dev/dvb/adapter%i/frontend%i", adapter, frontend);
	if ((fd = open(filename, flags)) < 0) {
		// if that failed, try a flat /dev structure
		sprintf(filename, "/dev/dvb%i.frontend%i", adapter, frontend);
		if ((fd = open(filename, flags)) < 0) {
			return NULL;
		}
	}

	// determine fe type
	if (ioctl(fd, FE_GET_INFO, &info)) {
		close(fd);
		return NULL;
	}

	// setup structure
	fehandle = (struct dvbfe_handle*) malloc(sizeof(struct dvbfe_handle));
	memset(fehandle, 0, sizeof(struct dvbfe_handle));
	fehandle->fd = fd;
	switch(info.type) {
	case FE_QPSK:
		fehandle->type = DVBFE_TYPE_DVBS;
		break;

	case FE_QAM:
		fehandle->type = DVBFE_TYPE_DVBC;
		break;

	case FE_OFDM:
		fehandle->type = DVBFE_TYPE_DVBT;
		break;

	case FE_ATSC:
		fehandle->type = DVBFE_TYPE_ATSC;
		break;
	}
	fehandle->name = strndup(info.name, sizeof(info.name));

	// done
	return fehandle;
}

void dvbfe_close(struct dvbfe_handle *fehandle)
{
	close(fehandle->fd);
	free(fehandle->name);
	free(fehandle);
}

extern int dvbfe_get_info(struct dvbfe_handle *fehandle,
			  enum dvbfe_info_mask querymask,
			  struct dvbfe_info *result,
			  enum dvbfe_info_querytype querytype,
			  int timeout)
{
	int returnval = 0;
	struct dvb_frontend_event kevent;
	int ok = 0;

	result->name = fehandle->name;
	result->type = fehandle->type;

	switch(querytype) {
	case DVBFE_INFO_QUERYTYPE_IMMEDIATE:
		if (querymask & DVBFE_INFO_LOCKSTATUS) {
			if (!ioctl(fehandle->fd, FE_READ_STATUS, &kevent.status)) {
				returnval |= DVBFE_INFO_LOCKSTATUS;
			}
		}
		if (querymask & DVBFE_INFO_FEPARAMS) {
			if (!ioctl(fehandle->fd, FE_GET_FRONTEND, &kevent.parameters)) {
				returnval |= DVBFE_INFO_FEPARAMS;
			}
		}
		break;

	case DVBFE_INFO_QUERYTYPE_LOCKCHANGE:
		{
			struct pollfd pollfd;
			pollfd.fd = fehandle->fd;
			pollfd.events = POLLIN | POLLERR;

			ok = 1;
			if (poll(&pollfd, 1, timeout) < 0)
				ok = 0;
			if (pollfd.revents & POLLERR)
				ok = 0;
			if (!(pollfd.revents & POLLIN))
				ok = 0;
		}

		if (ok &&
		    ((querymask & DVBFE_INFO_LOCKSTATUS) ||
		     (querymask & DVBFE_INFO_FEPARAMS))) {
			if (!ioctl(fehandle->fd, FE_GET_EVENT, &kevent)) {
				if (querymask & DVBFE_INFO_LOCKSTATUS)
					returnval |= DVBFE_INFO_LOCKSTATUS;
				if (querymask & DVBFE_INFO_FEPARAMS)
					returnval |= DVBFE_INFO_FEPARAMS;
			}
		}
		break;
	}

	if (returnval & DVBFE_INFO_LOCKSTATUS) {
		result->signal = kevent.status & FE_HAS_SIGNAL ? 1 : 0;
		result->carrier = kevent.status & FE_HAS_CARRIER ? 1 : 0;
		result->viterbi = kevent.status & FE_HAS_VITERBI ? 1 : 0;
		result->sync = kevent.status & FE_HAS_SYNC ? 1 : 0;
		result->lock = kevent.status & FE_HAS_LOCK ? 1 : 0;
	}

	if (returnval & DVBFE_INFO_FEPARAMS) {
		result->feparams.frequency = kevent.parameters.frequency;
		result->feparams.inversion = lookupval(kevent.parameters.inversion, 1, dvbfe_spectral_inversion_to_kapi);
		switch(fehandle->type) {
		case FE_QPSK:
			result->feparams.u.dvbs.symbol_rate = kevent.parameters.u.qpsk.symbol_rate;
			result->feparams.u.dvbs.fec_inner =
				lookupval(kevent.parameters.u.qpsk.fec_inner, 1, dvbfe_code_rate_to_kapi);
			break;

		case FE_QAM:
			result->feparams.u.dvbc.symbol_rate = kevent.parameters.u.qam.symbol_rate;
			result->feparams.u.dvbc.fec_inner =
				lookupval(kevent.parameters.u.qam.fec_inner, 1, dvbfe_code_rate_to_kapi);
			result->feparams.u.dvbc.modulation =
				lookupval(kevent.parameters.u.qam.modulation, 1, dvbfe_dvbc_mod_to_kapi);
			break;

		case FE_OFDM:
			result->feparams.u.dvbt.bandwidth =
				lookupval(kevent.parameters.u.ofdm.bandwidth, 1, dvbfe_dvbt_bandwidth_to_kapi);
			result->feparams.u.dvbt.code_rate_HP =
				lookupval(kevent.parameters.u.ofdm.code_rate_HP, 1, dvbfe_code_rate_to_kapi);
			result->feparams.u.dvbt.code_rate_LP =
				lookupval(kevent.parameters.u.ofdm.code_rate_LP, 1, dvbfe_code_rate_to_kapi);
			result->feparams.u.dvbt.constellation =
				lookupval(kevent.parameters.u.ofdm.constellation, 1, dvbfe_dvbt_const_to_kapi);
			result->feparams.u.dvbt.transmission_mode =
				lookupval(kevent.parameters.u.ofdm.transmission_mode, 1, dvbfe_dvbt_transmit_mode_to_kapi);
			result->feparams.u.dvbt.guard_interval =
				lookupval(kevent.parameters.u.ofdm.guard_interval, 1, dvbfe_dvbt_guard_interval_to_kapi);
			result->feparams.u.dvbt.hierarchy_information =
				lookupval(kevent.parameters.u.ofdm.hierarchy_information, 1, dvbfe_dvbt_hierarchy_to_kapi);
			break;

		case FE_ATSC:
			result->feparams.u.atsc.modulation =
				lookupval(kevent.parameters.u.vsb.modulation, 1, dvbfe_atsc_mod_to_kapi);
			break;
		}
	}

	if (querymask & DVBFE_INFO_BER) {
		if (!ioctl(fehandle->fd, FE_READ_BER, &result->ber))
			returnval |= DVBFE_INFO_BER;
	}
	if (querymask & DVBFE_INFO_SIGNAL_STRENGTH) {
		if (!ioctl(fehandle->fd, FE_READ_SIGNAL_STRENGTH, &result->signal_strength))
			returnval |= DVBFE_INFO_SIGNAL_STRENGTH;
	}
	if (querymask & DVBFE_INFO_SNR) {
		if (!ioctl(fehandle->fd, FE_READ_SNR, &result->snr))
			returnval |= DVBFE_INFO_SNR;
	}
	if (querymask & DVBFE_INFO_UNCORRECTED_BLOCKS) {
		if (!ioctl(fehandle->fd, FE_READ_UNCORRECTED_BLOCKS, &result->ucblocks))
			returnval |= DVBFE_INFO_UNCORRECTED_BLOCKS;
	}

	// done
	return returnval;
}

int dvbfe_set(struct dvbfe_handle *fehandle,
	      struct dvbfe_parameters *params,
	      int timeout)
{
	struct dvb_frontend_parameters kparams;
	int res;
	struct timeval endtime;
	fe_status_t status;

	kparams.frequency = params->frequency;
	kparams.inversion = lookupval(params->inversion, 0, dvbfe_spectral_inversion_to_kapi);
	switch(fehandle->type) {
	case FE_QPSK:
		kparams.u.qpsk.symbol_rate = params->u.dvbs.symbol_rate;
		kparams.u.qpsk.fec_inner = lookupval(params->u.dvbs.fec_inner, 0, dvbfe_code_rate_to_kapi);
		break;

	case FE_QAM:
		kparams.u.qam.symbol_rate = params->u.dvbc.symbol_rate;
		kparams.u.qam.fec_inner = lookupval(params->u.dvbc.fec_inner, 0, dvbfe_code_rate_to_kapi);
		kparams.u.qam.modulation = lookupval(params->u.dvbc.modulation, 0, dvbfe_dvbc_mod_to_kapi);
		break;

	case FE_OFDM:
		kparams.u.ofdm.bandwidth = lookupval(params->u.dvbt.bandwidth, 0, dvbfe_dvbt_bandwidth_to_kapi);
		kparams.u.ofdm.code_rate_HP = lookupval(params->u.dvbt.code_rate_HP, 0, dvbfe_code_rate_to_kapi);
		kparams.u.ofdm.code_rate_LP = lookupval(params->u.dvbt.code_rate_LP, 0, dvbfe_code_rate_to_kapi);
		kparams.u.ofdm.constellation = lookupval(params->u.dvbt.constellation, 0, dvbfe_dvbt_const_to_kapi);
		kparams.u.ofdm.transmission_mode =
			lookupval(params->u.dvbt.transmission_mode, 0, dvbfe_dvbt_transmit_mode_to_kapi);
		kparams.u.ofdm.guard_interval =
			lookupval(params->u.dvbt.guard_interval, 0, dvbfe_dvbt_guard_interval_to_kapi);
		kparams.u.ofdm.hierarchy_information =
			lookupval(params->u.dvbt.hierarchy_information, 0, dvbfe_dvbt_hierarchy_to_kapi);
                break;

	case FE_ATSC:
		kparams.u.vsb.modulation = lookupval(params->u.atsc.modulation, 0, dvbfe_atsc_mod_to_kapi);
		break;

	default:
		return -EINVAL;
	}

	// set it and check for error
	res = ioctl(fehandle->fd, FE_SET_FRONTEND, &kparams);
	if (res)
		return res;

	// 0 => return immediately
	if (timeout == 0) {
		return 0;
	}

	/* calculate timeout */
	if (timeout > 0) {
		gettimeofday(&endtime, NULL);
		timeout *= 1000;
		endtime.tv_sec += timeout / 1000000;
		endtime.tv_usec += timeout % 1000000;
	}

	/* wait for a lock */
	while(1) {
		/* has it locked? */
		if (!ioctl(fehandle->fd, FE_READ_STATUS, &status)) {
			if (status & FE_HAS_LOCK) {
				break;
			}
		}

		/* check for timeout */
		if (timeout > 0) {
			struct timeval curtime;
			gettimeofday(&curtime, NULL);
			if ((curtime.tv_sec > endtime.tv_sec) ||
			    ((curtime.tv_sec == endtime.tv_sec) && (curtime.tv_usec >= endtime.tv_usec))) {
				break;
			}
		}

		/* delay for a bit */
		usleep(100000);
	}

	/* exit */
	if (status & FE_HAS_LOCK)
		return 0;
	return -ETIMEDOUT;
}

int dvbfe_get_pollfd(struct dvbfe_handle *handle)
{
	return handle->fd;
}

int dvbfe_set_22k_tone(struct dvbfe_handle *fehandle, enum dvbfe_sec_tone_mode tone)
{
	int ret = 0;

	switch (tone) {
	case DVBFE_SEC_TONE_OFF:
		ret = ioctl(fehandle->fd, FE_SET_TONE, SEC_TONE_OFF);
		break;
	case DVBFE_SEC_TONE_ON:
		ret = ioctl(fehandle->fd, FE_SET_TONE, SEC_TONE_ON);
		break;
	default:
		print(verbose, ERROR, 1, "Invalid command !");
		break;
	}
	if (ret == -1)
		print(verbose, ERROR, 1, "IOCTL failed !");

	return ret;
}

int dvbfe_set_tone_data_burst(struct dvbfe_handle *fehandle, enum dvbfe_sec_mini_cmd minicmd)
{
	int ret = 0;

	switch (minicmd) {
	case DVBFE_SEC_MINI_A:
		ret = ioctl(fehandle->fd, FE_DISEQC_SEND_BURST, SEC_MINI_A);
		break;
	case DVBFE_SEC_MINI_B:
		ret = ioctl(fehandle->fd, FE_DISEQC_SEND_BURST, SEC_MINI_B);
		break;
	default:
		print(verbose, ERROR, 1, "Invalid command");
		break;
	}
	if (ret == -1)
		print(verbose, ERROR, 1, "IOCTL failed");

	return ret;
}

int dvbfe_set_voltage(struct dvbfe_handle *fehandle, enum dvbfe_sec_voltage voltage)
{
	int ret = 0;

	switch (voltage) {
	case DVBFE_SEC_VOLTAGE_OFF:
		ret = ioctl(fehandle->fd, FE_SET_VOLTAGE, SEC_VOLTAGE_OFF);
		break;
	case DVBFE_SEC_VOLTAGE_13:
		ret = ioctl(fehandle->fd, FE_SET_VOLTAGE, SEC_VOLTAGE_13);
		break;
	case DVBFE_SEC_VOLTAGE_18:
		ret = ioctl(fehandle->fd, FE_SET_VOLTAGE, SEC_VOLTAGE_18);
		break;
	default:
		print(verbose, ERROR, 1, "Invalid command");
		break;
	}
	if (ret == -1)
		print(verbose, ERROR, 1, "IOCTL failed");

	return ret;
}

int dvbfe_set_high_lnb_voltage(struct dvbfe_handle *fehandle, int on)
{
	switch (on) {
	case 0:
		ioctl(fehandle->fd, FE_ENABLE_HIGH_LNB_VOLTAGE, 0);
		break;
	default:
		ioctl(fehandle->fd, FE_ENABLE_HIGH_LNB_VOLTAGE, 1);
		break;
	}
	return 0;
}

int dvbfe_do_dishnetworks_legacy_command(struct dvbfe_handle *fehandle, unsigned int cmd)
{
	int ret = 0;

	ret = ioctl(fehandle->fd, FE_DISHNETWORK_SEND_LEGACY_CMD, cmd);
	if (ret == -1)
		print(verbose, ERROR, 1, "IOCTL failed");

	return ret;
}

int dvbfe_do_diseqc_command(struct dvbfe_handle *fehandle, uint8_t *data, uint8_t len)
{
	int ret = 0;
	struct dvb_diseqc_master_cmd diseqc_message;

	if (len > 6)
		return -EINVAL;

	diseqc_message.msg_len = len;
	memcpy(diseqc_message.msg, data, len);

	ret = ioctl(fehandle->fd, FE_DISEQC_SEND_MASTER_CMD, &diseqc_message);
	if (ret == -1)
		print(verbose, ERROR, 1, "IOCTL failed");

	return ret;
}

int dvbfe_diseqc_read(struct dvbfe_handle *fehandle, int timeout, unsigned char *buf, unsigned int len)
{
	struct dvb_diseqc_slave_reply reply;
	int result;

	if (len > 4)
		len = 4;

	reply.timeout = timeout;
	reply.msg_len = len;

	if ((result = ioctl(fehandle->fd, FE_DISEQC_RECV_SLAVE_REPLY, reply)) != 0)
		return result;

	if (reply.msg_len < len)
		len = reply.msg_len;
	memcpy(buf, reply.msg, len);

	return len;
}
