/*
	libdvbsec - an SEC library

	Copyright (C) 2005 Manu Abraham <abraham.manu@gmail.com>
	Copyright (C) 2006 Andrew de Quincey <adq_dvb@lidskialf.net>

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.
	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef DVBSEC_API_H
#define DVBSEC_API_H 1

#include <stdint.h>

struct dvbfe_handle;
struct dvbfe_parameters;

enum dvbsec_diseqc_framing {
	DISEQC_FRAMING_MASTER_NOREPLY		= 0xE0,
	DISEQC_FRAMING_MASTER_NOREPLY_REPEAT	= 0xE1,
	DISEQC_FRAMING_MASTER_REPLY		= 0xE2,
	DISEQC_FRAMING_MASTER_REPLY_REPEAT	= 0xE3,
	DISEQC_FRAMING_SLAVE_OK			= 0xE4,
	DISEQC_FRAMING_SLAVE_UNSUPPORTED	= 0xE5,
	DISEQC_FRAMING_SLAVE_PARITY_ERROR	= 0xE6,
	DISEQC_FRAMING_SLAVE_UNRECOGNISED 	= 0xE7,
};

enum dvbsec_diseqc_address {
	DISEQC_ADDRESS_ANY_DEVICE		= 0x00,

	DISEQC_ADDRESS_ANY_LNB_SWITCHER_SMATV	= 0x10,
	DISEQC_ADDRESS_LNB			= 0x11,
	DISEQC_ADDRESS_LNB_WITH_LOOP		= 0x12,
	DISEQC_ADDRESS_SWITCHER			= 0x14,
	DISEQC_ADDRESS_SWITCHER_WITH_LOOP	= 0x15,
	DISEQC_ADDRESS_SMATV			= 0x18,

	DISEQC_ADDRESS_ANY_POLARISER		= 0x20,
	DISEQC_ADDRESS_LINEAR_POLARISER		= 0x21,

	DISEQC_ADDRESS_ANY_POSITIONER		= 0x30,
	DISEQC_ADDRESS_POLAR_AZIMUTH_POSITIONER	= 0x31,
	DISEQC_ADDRESS_ELEVATION_POSITIONER	= 0x32,

	DISEQC_ADDRESS_ANY_INSTALLER_AID	= 0x40,
	DISEQC_ADDRESS_SIGNAL_STRENGTH		= 0x41,

	DISEQC_ADDRESS_ANY_INTERFACE		= 0x70,
	DISEQC_ADDRESS_HEADEND_INTERFACE	= 0x71,

	DISEQC_ADDRESS_REALLOC_BASE		= 0x60,
	DISEQC_ADDRESS_OEM_BASE			= 0xf0,
};

enum dvbsec_diseqc_reset {
	DISEQC_RESET,
	DISEQC_RESET_CLEAR,
};

enum dvbsec_diseqc_power {
	DISEQC_POWER_OFF,
	DISEQC_POWER_ON,
};

enum dvbsec_diseqc_listen {
	DISEQC_LISTEN_SLEEP,
	DISEQC_LISTEN_AWAKE,
};

enum dvbsec_diseqc_polarization {
	DISEQC_POLARIZATION_UNCHANGED = 0,
	DISEQC_POLARIZATION_H = 'h',
	DISEQC_POLARIZATION_V = 'v',
	DISEQC_POLARIZATION_L = 'l',
	DISEQC_POLARIZATION_R = 'r',
};

enum dvbsec_diseqc_oscillator {
	DISEQC_OSCILLATOR_UNCHANGED = 0,
	DISEQC_OSCILLATOR_LOW,
	DISEQC_OSCILLATOR_HIGH,
};

enum dvbsec_diseqc_switch {
	DISEQC_SWITCH_UNCHANGED = 0,
	DISEQC_SWITCH_A,
	DISEQC_SWITCH_B,
};

enum dvbsec_diseqc_analog_id {
	DISEQC_ANALOG_ID_A0,
	DISEQC_ANALOG_ID_A1,
};

enum dvbsec_diseqc_drive_mode {
	DISEQC_DRIVE_MODE_STEPS,
	DISEQC_DRIVE_MODE_TIMEOUT,
};

enum dvbsec_diseqc_direction {
	DISEQC_DIRECTION_EAST,
	DISEQC_DIRECTION_WEST,
};

enum dvbsec_config_type {
	DVBSEC_CONFIG_NONE = 0,
	DVBSEC_CONFIG_POWER,
	DVBSEC_CONFIG_STANDARD,
	DVBSEC_CONFIG_ADVANCED,
};


#define MAX_SEC_CMD_LEN 100

struct dvbsec_config
{
	char id[32]; /* ID of this SEC config structure */
	uint32_t switch_frequency; /* switching frequency - supply 0 for none. */
	uint32_t lof_lo_v; /* frequency to subtract for V + LOW band channels - or for switch_frequency == 0 */
	uint32_t lof_lo_h; /* frequency to subtract for H + LOW band channels - or for switch_frequency == 0 */
	uint32_t lof_lo_l; /* frequency to subtract for L + LOW band channels - or for switch_frequency == 0 */
	uint32_t lof_lo_r; /* frequency to subtract for R + LOW band channels - or for switch_frequency == 0 */
	uint32_t lof_hi_v; /* frequency to subtract for V + HIGH band channels */
	uint32_t lof_hi_h; /* frequency to subtract for H + HIGH band channels */
	uint32_t lof_hi_l; /* frequency to subtract for L + HIGH band channels */
	uint32_t lof_hi_r; /* frequency to subtract for R + HIGH band channels */

	/**
	 * The SEC control to be used depends on the type:
	 *
	 * NONE - no SEC commands will be issued. (Frequency adjustment will still be performed).
	 *
	 * POWER - only the SEC power will be turned on.
	 *
	 * STANDARD - the standard DISEQC back compatable sequence is used.
	 *
	 * ADVANCED - SEC strings are supplied by the user describing the exact sequence
	 * of operations to use.
	 */
	enum dvbsec_config_type config_type;

	/* stuff for type == dvbsec_config_ADVANCED */
	char adv_cmd_lo_h[MAX_SEC_CMD_LEN];			/* ADVANCED SEC command to use for LOW/H. */
	char adv_cmd_lo_v[MAX_SEC_CMD_LEN];			/* ADVANCED SEC command to use for LOW/V. */
	char adv_cmd_lo_l[MAX_SEC_CMD_LEN];			/* ADVANCED SEC command to use for LOW/L. */
	char adv_cmd_lo_r[MAX_SEC_CMD_LEN];			/* ADVANCED SEC command to use for LOW/R. */
	char adv_cmd_hi_h[MAX_SEC_CMD_LEN];			/* ADVANCED SEC command to use for HI/H. */
	char adv_cmd_hi_v[MAX_SEC_CMD_LEN];			/* ADVANCED SEC command to use for HI/V. */
	char adv_cmd_hi_l[MAX_SEC_CMD_LEN];			/* ADVANCED SEC command to use for HI/L. */
	char adv_cmd_hi_r[MAX_SEC_CMD_LEN];			/* ADVANCED SEC command to use for HI/R. */
};

/**
 * Helper function for tuning adapters with SEC support. This function will do
 * everything required, including frequency adjustment based on the parameters
 * in sec_config.
 *
 * Note: Since the SEC configuration structure can be set to disable any SEC
 * operations, this function can be reused for ALL DVB style devices (just
 * set all LOF=0,type=dvbsec_config_NONE for devices which do not require
 * SEC control).
 *
 * The sec configuration structures can be looked up using the dvbcfg_sec library.
 *
 * @param fe Frontend concerned.
 * @param sec_config SEC configuration structure. May be NULL to disable SEC/frequency adjustment.
 * @param polarization Polarization of signal.
 * @param sat_pos Satellite position - only used if type == DISEQC_SEC_CONFIG_STANDARD.
 * @param switch_option Switch option - only used if type == DISEQC_SEC_CONFIG_STANDARD.
 * @param params Tuning parameters.
 * @param timeout <0 => wait forever for lock. 0=>return immediately, >0=>
 * number of milliseconds to wait for a lock.
 * @return 0 on locked (or if timeout==0 and everything else worked), or
 * nonzero on failure (including no lock).
 */
extern int dvbsec_set(struct dvbfe_handle *fe,
			  struct dvbsec_config *sec_config,
			  enum dvbsec_diseqc_polarization polarization,
			  enum dvbsec_diseqc_switch sat_pos,
			  enum dvbsec_diseqc_switch switch_option,
			  struct dvbfe_parameters *params,
			  int timeout);

/**
 * This will issue the standardised back-compatable DISEQC/SEC command
 * sequence as defined in the DISEQC spec:
 *
 * i.e. tone off, set voltage, wait15, DISEQC, wait15, toneburst, wait15, set tone.
 *
 * @param fe Frontend concerned.
 * @param oscillator Value to set the lo/hi switch to.
 * @param polarization Value to set the polarisation switch to.
 * @param sat_pos Value to set the satellite position switch to.
 * @param switch_option Value to set the "swtch option" switch to.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_std_sequence(struct dvbfe_handle *fe,
				  enum dvbsec_diseqc_oscillator oscillator,
				  enum dvbsec_diseqc_polarization polarization,
				  enum dvbsec_diseqc_switch sat_pos,
				  enum dvbsec_diseqc_switch switch_option);

/**
 * Execute an SEC command string on the provided frontend. Please see the documentation
 * in dvbsec_cfg.h on the command format,
 *
 * @param fe Frontend concerned.
 * @param command The command to execute.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_command(struct dvbfe_handle *fe, char *command);

/**
 * Control the reset status of an attached DISEQC device.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param state The state to set.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_set_reset(struct dvbfe_handle *fe,
				  enum dvbsec_diseqc_address address,
				  enum dvbsec_diseqc_reset state);

/**
 * Control the power status of an attached DISEQC peripheral.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param state The state to set.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_set_power(struct dvbfe_handle *fe,
				  enum dvbsec_diseqc_address address,
				  enum dvbsec_diseqc_power state);

/**
 * Control the listening status of an attached DISEQC peripheral.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param state The state to set.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_set_listen(struct dvbfe_handle *fe,
				   enum dvbsec_diseqc_address address,
				   enum dvbsec_diseqc_listen state);

/**
 * Set the state of the committed switches of a DISEQC device.
 * These are switches which are defined to have a standard name.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param oscillator Value to set the lo/hi switch to.
 * @param polarization Value to set the polarization switch to.
 * @param sat_pos Value to set the satellite position switch to.
 * @param switch_option Value to set the switch option switch to.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_set_committed_switches(struct dvbfe_handle *fe,
					       enum dvbsec_diseqc_address address,
					       enum dvbsec_diseqc_oscillator oscillator,
					       enum dvbsec_diseqc_polarization polarization,
					       enum dvbsec_diseqc_switch sat_pos,
					       enum dvbsec_diseqc_switch switch_option);

/**
 * Set the state of the uncommitted switches of a DISEQC device.
 * These provide another four switching possibilities.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param s1 Value to set the S1 switch to.
 * @param s2 Value to set the S2 switch to.
 * @param s3 Value to set the S3 switch to.
 * @param s3 Value to set the S4 switch to.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_set_uncommitted_switches(struct dvbfe_handle *fe,
						 enum dvbsec_diseqc_address address,
						 enum dvbsec_diseqc_switch s1,
						 enum dvbsec_diseqc_switch s2,
						 enum dvbsec_diseqc_switch s3,
						 enum dvbsec_diseqc_switch s4);

/**
 * Set an analogue value.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param id The id of the analogue value to set.
 * @param value The value to set.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_set_analog_value(struct dvbfe_handle *fe,
					 enum dvbsec_diseqc_address address,
					 enum dvbsec_diseqc_analog_id id,
					 uint8_t value);

/**
 * Set the desired frequency.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param frequency The frequency to set in GHz.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_set_frequency(struct dvbfe_handle *fe,
				      enum dvbsec_diseqc_address address,
				      uint32_t frequency);

/**
 * Set the desired channel.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param channel ID of the channel to set.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_set_channel(struct dvbfe_handle *fe,
				    enum dvbsec_diseqc_address address,
				    uint16_t channel);

/**
 * Halt the satellite positioner.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_halt_satpos(struct dvbfe_handle *fe,
				    enum dvbsec_diseqc_address address);

/**
 * Disable satellite positioner limits.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_disable_satpos_limits(struct dvbfe_handle *fe,
					      enum dvbsec_diseqc_address address);

/**
 * Set satellite positioner limits.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_set_satpos_limit(struct dvbfe_handle *fe,
					 enum dvbsec_diseqc_address address,
					 enum dvbsec_diseqc_direction direction);

/**
 * Drive satellite positioner motor.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param direction Direction to drive in.
 * @param mode Drive mode to use
 * 	       (TIMEOUT=>value is a timeout in seconds, or STEPS=>value is a count of steps to use)
 * @param value Value associated with the drive mode (range 0->127)
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_drive_satpos_motor(struct dvbfe_handle *fe,
					   enum dvbsec_diseqc_address address,
					   enum dvbsec_diseqc_direction direction,
					   enum dvbsec_diseqc_drive_mode mode,
					   uint8_t value);

/**
 * Store satellite positioner preset id at current position.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param id ID of the preset.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_store_satpos_preset(struct dvbfe_handle *fe,
					    enum dvbsec_diseqc_address address,
					    uint8_t id);

/**
 * Send a satellite positioner to a pre-set position.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param id ID of the preset.
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_goto_satpos_preset(struct dvbfe_handle *fe,
					   enum dvbsec_diseqc_address address,
					   uint8_t id);

/**
 * Recalculate satellite positions based on the current position, using
 * magic positioner specific values.
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param val1 value1 (range 0->255, pass -1 to ignore).
 * @param val2 value2 (range 0->255, pass -1 to ignore).
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_recalculate_satpos_positions(struct dvbfe_handle *fe,
						     enum dvbsec_diseqc_address address,
						     int val1,
						     int val2);

/**
 * Send a terrestrial aerial rotator to a particular bearing
 * (0 degrees = north, fractional angles allowed).
 *
 * @param fe Frontend concerned.
 * @param address Address of the device.
 * @param angle Angle to rotate to (-256.0 -> 512.0)
 * @return 0 on success, or nonzero on error.
 */
extern int dvbsec_diseqc_goto_rotator_bearing(struct dvbfe_handle *fe,
					     enum dvbsec_diseqc_address address,
					     float angle);

#endif
