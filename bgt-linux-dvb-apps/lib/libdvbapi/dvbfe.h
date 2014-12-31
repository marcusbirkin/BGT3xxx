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

#ifndef LIBDVBFE_H
#define LIBDVBFE_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * The types of frontend we support.
 */
enum dvbfe_type {
	DVBFE_TYPE_DVBS,
	DVBFE_TYPE_DVBC,
	DVBFE_TYPE_DVBT,
	DVBFE_TYPE_ATSC,
};

enum dvbfe_spectral_inversion {
	DVBFE_INVERSION_OFF,
	DVBFE_INVERSION_ON,
	DVBFE_INVERSION_AUTO
};

enum dvbfe_code_rate {
	DVBFE_FEC_NONE,
	DVBFE_FEC_1_2,
	DVBFE_FEC_2_3,
	DVBFE_FEC_3_4,
	DVBFE_FEC_4_5,
	DVBFE_FEC_5_6,
	DVBFE_FEC_6_7,
	DVBFE_FEC_7_8,
	DVBFE_FEC_8_9,
	DVBFE_FEC_AUTO
};

enum dvbfe_dvbt_const {
	DVBFE_DVBT_CONST_QPSK,
	DVBFE_DVBT_CONST_QAM_16,
	DVBFE_DVBT_CONST_QAM_32,
	DVBFE_DVBT_CONST_QAM_64,
	DVBFE_DVBT_CONST_QAM_128,
	DVBFE_DVBT_CONST_QAM_256,
	DVBFE_DVBT_CONST_AUTO
};

enum dvbfe_dvbc_mod {
	DVBFE_DVBC_MOD_QAM_16,
	DVBFE_DVBC_MOD_QAM_32,
	DVBFE_DVBC_MOD_QAM_64,
	DVBFE_DVBC_MOD_QAM_128,
	DVBFE_DVBC_MOD_QAM_256,
	DVBFE_DVBC_MOD_AUTO,
};

enum dvbfe_atsc_mod {
	DVBFE_ATSC_MOD_QAM_64,
	DVBFE_ATSC_MOD_QAM_256,
	DVBFE_ATSC_MOD_VSB_8,
	DVBFE_ATSC_MOD_VSB_16,
	DVBFE_ATSC_MOD_AUTO
};

enum dvbfe_dvbt_transmit_mode {
	DVBFE_DVBT_TRANSMISSION_MODE_2K,
	DVBFE_DVBT_TRANSMISSION_MODE_8K,
	DVBFE_DVBT_TRANSMISSION_MODE_AUTO
};

enum dvbfe_dvbt_bandwidth {
	DVBFE_DVBT_BANDWIDTH_8_MHZ,
	DVBFE_DVBT_BANDWIDTH_7_MHZ,
	DVBFE_DVBT_BANDWIDTH_6_MHZ,
	DVBFE_DVBT_BANDWIDTH_AUTO
};

enum dvbfe_dvbt_guard_interval {
	DVBFE_DVBT_GUARD_INTERVAL_1_32,
	DVBFE_DVBT_GUARD_INTERVAL_1_16,
	DVBFE_DVBT_GUARD_INTERVAL_1_8,
	DVBFE_DVBT_GUARD_INTERVAL_1_4,
	DVBFE_DVBT_GUARD_INTERVAL_AUTO
};

enum dvbfe_dvbt_hierarchy {
	DVBFE_DVBT_HIERARCHY_NONE,
	DVBFE_DVBT_HIERARCHY_1,
	DVBFE_DVBT_HIERARCHY_2,
	DVBFE_DVBT_HIERARCHY_4,
	DVBFE_DVBT_HIERARCHY_AUTO
};

/**
 * Structure used to store and communicate frontend parameters.
 */
struct dvbfe_parameters {
	uint32_t frequency;
	enum dvbfe_spectral_inversion inversion;
	union {
		struct {
			uint32_t			symbol_rate;
			enum dvbfe_code_rate		fec_inner;
		} dvbs;

		struct {
			uint32_t			symbol_rate;
			enum dvbfe_code_rate		fec_inner;
			enum dvbfe_dvbc_mod		modulation;
		} dvbc;

		struct {
			enum dvbfe_dvbt_bandwidth	bandwidth;
			enum dvbfe_code_rate		code_rate_HP;
			enum dvbfe_code_rate		code_rate_LP;
			enum dvbfe_dvbt_const		constellation;
			enum dvbfe_dvbt_transmit_mode	transmission_mode;
			enum dvbfe_dvbt_guard_interval	guard_interval;
			enum dvbfe_dvbt_hierarchy	hierarchy_information;
		} dvbt;

		struct {
			enum dvbfe_atsc_mod		modulation;
		} atsc;
	} u;
};

enum dvbfe_sec_voltage {
	DVBFE_SEC_VOLTAGE_13,
	DVBFE_SEC_VOLTAGE_18,
	DVBFE_SEC_VOLTAGE_OFF
};

enum dvbfe_sec_tone_mode {
	DVBFE_SEC_TONE_ON,
	DVBFE_SEC_TONE_OFF
};

enum dvbfe_sec_mini_cmd {
	DVBFE_SEC_MINI_A,
	DVBFE_SEC_MINI_B
};

/**
 * Mask of values used in the dvbfe_get_info() call.
 */
enum dvbfe_info_mask {
	DVBFE_INFO_LOCKSTATUS			= 0x01,
	DVBFE_INFO_FEPARAMS			= 0x02,
	DVBFE_INFO_BER				= 0x04,
	DVBFE_INFO_SIGNAL_STRENGTH		= 0x08,
	DVBFE_INFO_SNR				= 0x10,
	DVBFE_INFO_UNCORRECTED_BLOCKS		= 0x20,
};

/**
 * Structure containing values used by the dvbfe_get_info() call.
 */
struct dvbfe_info {
	enum dvbfe_type type;			/* always retrieved */
	const char *name;			/* always retrieved */
	unsigned int signal     : 1;		/* } DVBFE_INFO_LOCKSTATUS */
	unsigned int carrier    : 1;		/* } */
	unsigned int viterbi    : 1;		/* } */
	unsigned int sync       : 1;		/* } */
	unsigned int lock       : 1;		/* } */
	struct dvbfe_parameters feparams;	/* DVBFE_INFO_FEPARAMS */
	uint32_t ber;				/* DVBFE_INFO_BER */
	uint16_t signal_strength;		/* DVBFE_INFO_SIGNAL_STRENGTH */
	uint16_t snr;				/* DVBFE_INFO_SNR */
	uint32_t ucblocks;			/* DVBFE_INFO_UNCORRECTED_BLOCKS */
};

/**
 * Possible types of query used in dvbfe_get_info.
 *
 * DVBFE_INFO_QUERYTYPE_IMMEDIATE  - interrogate frontend for most up to date values.
 * DVBFE_INFO_QUERYTYPE_LOCKCHANGE - return details from queued lock status
 * 				     change events, or wait for one to occur
 * 				     if none are queued.
 */
enum dvbfe_info_querytype {
	DVBFE_INFO_QUERYTYPE_IMMEDIATE,
	DVBFE_INFO_QUERYTYPE_LOCKCHANGE,
};


/**
 * Frontend handle datatype.
 */
struct dvbfe_handle;

/**
 * Open a DVB frontend.
 *
 * @param adapter DVB adapter ID.
 * @param frontend Frontend ID of that adapter to open.
 * @param readonly If 1, frontend will be opened in readonly mode only.
 * @return A handle on success, or NULL on failure.
 */
extern struct dvbfe_handle *dvbfe_open(int adapter, int frontend, int readonly);

/**
 * Close a DVB frontend.
 *
 * @param fehandle Handle opened with dvbfe_open().
 */
extern void dvbfe_close(struct dvbfe_handle *handle);

/**
 * Set the frontend tuning parameters.
 *
 * Note: this function provides only the basic tuning operation; you might want to
 * investigate dvbfe_set_sec() in sec.h for a unified device tuning operation.
 *
 * @param fehandle Handle opened with dvbfe_open().
 * @param params Params to set.
 * @param timeout <0 => wait forever for lock. 0=>return immediately, >0=>
 * number of milliseconds to wait for a lock.
 * @return 0 on locked (or if timeout==0 and everything else worked), or
 * nonzero on failure (including no lock).
 */
extern int dvbfe_set(struct dvbfe_handle *fehandle,
		     struct dvbfe_parameters *params,
		     int timeout);

/**
 * Retrieve information about the frontend.
 *
 * @param fehandle Handle opened with dvbfe_open().
 * @param querymask ORed bitmask of desired DVBFE_INFO_* values.
 * @param result Where to put the retrieved results.
 * @param querytype Type of query requested.
 * @param timeout Timeout in ms to use if querytype==lockchange (0=>no timeout, <0=> wait forever).
 * @return ORed bitmask of DVBFE_INFO_* indicating which values were read successfully.
 */
extern int dvbfe_get_info(struct dvbfe_handle *fehandle,
			  enum dvbfe_info_mask querymask,
			  struct dvbfe_info *result,
			  enum dvbfe_info_querytype querytype,
			  int timeout);

/**
 * Get a file descriptor for polling for lock status changes.
 *
 * @param fehandle Handle opened with dvbfe_open().
 * @return FD for polling.
 */
extern int dvbfe_get_pollfd(struct dvbfe_handle *handle);

/**
 *	Tone/Data Burst control
 * 	@param fehandle Handle opened with dvbfe_open().
 *	@param tone, SEC_TONE_ON/SEC_TONE_OFF
 */
extern int dvbfe_set_22k_tone(struct dvbfe_handle *handle, enum dvbfe_sec_tone_mode tone);

/**
 *	22khz Tone control
 * 	@param fehandle Handle opened with dvbfe_open().
 *	@param adapter, minicmd, SEC_MINI_A/SEC_MINI_B
 */
extern int dvbfe_set_tone_data_burst(struct dvbfe_handle *handle, enum dvbfe_sec_mini_cmd minicmd);

/**
 *	Voltage control
 * 	@param fehandle Handle opened with dvbfe_open().
 *	@param polarization, SEC_VOLTAGE_13/SEC_VOLTAGE_18/SEC_VOLTAGE_OFF
 */
extern int dvbfe_set_voltage(struct dvbfe_handle *handle, enum dvbfe_sec_voltage voltage);

/**
 *	High LNB voltage control (increases voltage by 1v to compensate for long cables)
 * 	@param fehandle Handle opened with dvbfe_open().
 *	@param on 1 to enable, 0 to disable.
 */
extern int dvbfe_set_high_lnb_voltage(struct dvbfe_handle *fehandle, int on);

/**
 *	Send a legacy Dish Networks command
 * 	@param fehandle Handle opened with dvbfe_open().
 *	@param cmd, the command to send
 */
extern int dvbfe_do_dishnetworks_legacy_command(struct dvbfe_handle *handle, unsigned int cmd);

/**
 *	Send a DiSEqC Command
 * 	@param fehandle Handle opened with dvbfe_open().
 *	@param data, a pointer to am array containing the data to be sent.
 *      @param len Length of data in  bytes, max 6 bytes.
 */
extern int dvbfe_do_diseqc_command(struct dvbfe_handle *handle, uint8_t *data, uint8_t len);

/**
 * Read a DISEQC response from the frontend.
 *
 * @param fehandle Handle opened with dvbfe_open().
 * @param timeout Timeout for DISEQC response.
 * @param buf Buffer to store response in.
 * @param len Number of bytes in buffer.
 * @return >= 0 on success (number of received bytes), <0 on failure.
 */
extern int dvbfe_diseqc_read(struct dvbfe_handle *fehandle, int timeout, unsigned char *buf, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif // LIBDVBFE_H
