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

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <linux/types.h>
#include <libdvbapi/dvbfe.h>
#include "dvbsec_api.h"

// uncomment this to make dvbsec_command print out debug instead of talking to a frontend
// #define TEST_SEC_COMMAND 1

int dvbsec_set(struct dvbfe_handle *fe,
		   struct dvbsec_config *sec_config,
		   enum dvbsec_diseqc_polarization polarization,
		   enum dvbsec_diseqc_switch sat_pos,
		   enum dvbsec_diseqc_switch switch_option,
		   struct dvbfe_parameters *params,
		   int timeout)
{
	int tmp;
	struct dvbfe_parameters localparams;
	struct dvbfe_parameters *topass = params;

	// perform SEC
	if (sec_config != NULL) {
		switch(sec_config->config_type) {
		case DVBSEC_CONFIG_NONE:
			break;

		case DVBSEC_CONFIG_POWER:
			dvbfe_set_voltage(fe, DVBFE_SEC_VOLTAGE_13);
			break;

		case DVBSEC_CONFIG_STANDARD:
		{
			// calculate the correct oscillator value
			enum dvbsec_diseqc_oscillator osc = DISEQC_OSCILLATOR_LOW;
			if (sec_config->switch_frequency && (sec_config->switch_frequency < params->frequency))
				osc = DISEQC_OSCILLATOR_HIGH;

			if ((tmp = dvbsec_std_sequence(fe,
			     				  osc,
							  polarization,
							  sat_pos,
							  switch_option)) < 0)
				return tmp;
			break;
		}

		case DVBSEC_CONFIG_ADVANCED:
		{
			// are we high or not?
			int high = 0;
			if (sec_config->switch_frequency && (sec_config->switch_frequency < params->frequency))
				high = 1;

			//  determine correct string
			char *cmd = NULL;
			switch(polarization) {
			case DISEQC_POLARIZATION_H:
				if (!high)
					cmd = sec_config->adv_cmd_lo_h;
				else
					cmd = sec_config->adv_cmd_hi_h;
				break;
			case DISEQC_POLARIZATION_V:
				if (!high)
					cmd = sec_config->adv_cmd_lo_v;
				else
					cmd = sec_config->adv_cmd_hi_v;
				break;
			case DISEQC_POLARIZATION_L:
				if (!high)
					cmd = sec_config->adv_cmd_lo_l;
				else
					cmd = sec_config->adv_cmd_hi_l;
				break;
			case DISEQC_POLARIZATION_R:
				if (!high)
					cmd = sec_config->adv_cmd_lo_r;
				else
					cmd = sec_config->adv_cmd_hi_r;
				break;
			default:
				return -EINVAL;
			}

			// do it
			if (cmd)
				if ((tmp = dvbsec_command(fe, cmd)) < 0)
					return tmp;
			break;
		}
		}

		// work out the correct LOF value
		uint32_t lof = 0;
		if ((sec_config->switch_frequency == 0) || (params->frequency < sec_config->switch_frequency)) {
			// LOW band
			switch(polarization) {
			case DISEQC_POLARIZATION_H:
				lof = sec_config->lof_lo_h;
				break;
			case DISEQC_POLARIZATION_V:
				lof = sec_config->lof_lo_v;
				break;
			case DISEQC_POLARIZATION_L:
				lof = sec_config->lof_lo_l;
				break;
			case DISEQC_POLARIZATION_R:
				lof = sec_config->lof_lo_r;
				break;
			case DISEQC_POLARIZATION_UNCHANGED:
				break;
			}
		} else {
			// HIGH band
			switch(polarization) {
			case DISEQC_POLARIZATION_H:
				lof = sec_config->lof_hi_h;
				break;
			case DISEQC_POLARIZATION_V:
				lof = sec_config->lof_hi_v;
				break;
			case DISEQC_POLARIZATION_L:
				lof = sec_config->lof_hi_l;
				break;
			case DISEQC_POLARIZATION_R:
				lof = sec_config->lof_hi_r;
				break;
			case DISEQC_POLARIZATION_UNCHANGED:
				break;
			}
		}

		// do frequency adjustment
		if (lof) {
			memcpy(&localparams, params, sizeof(struct dvbfe_parameters));
			int tmpfreq = localparams.frequency - lof;

			if (tmpfreq < 0)
				tmpfreq *= -1;
			localparams.frequency = (uint32_t) tmpfreq;
			topass = &localparams;
		}
	}

	// set the frontend!
	return dvbfe_set(fe, topass, timeout);
}

int dvbsec_std_sequence(struct dvbfe_handle *fe,
			   enum dvbsec_diseqc_oscillator oscillator,
			   enum dvbsec_diseqc_polarization polarization,
			   enum dvbsec_diseqc_switch sat_pos,
			   enum dvbsec_diseqc_switch switch_option)
{
	dvbfe_set_22k_tone(fe, DVBFE_SEC_TONE_OFF);

	switch(polarization) {
	case DISEQC_POLARIZATION_V:
	case DISEQC_POLARIZATION_R:
		dvbfe_set_voltage(fe, DVBFE_SEC_VOLTAGE_13);
		break;
	case DISEQC_POLARIZATION_H:
	case DISEQC_POLARIZATION_L:
		dvbfe_set_voltage(fe, DVBFE_SEC_VOLTAGE_18);
		break;
	default:
		return -EINVAL;
	}

	dvbsec_diseqc_set_committed_switches(fe,
					    DISEQC_ADDRESS_ANY_DEVICE,
					    oscillator,
					    polarization,
					    sat_pos,
					    switch_option);

	usleep(15000);

	switch(sat_pos) {
	case DISEQC_SWITCH_A:
		dvbfe_set_tone_data_burst(fe, DVBFE_SEC_MINI_A);
		break;
	case DISEQC_SWITCH_B:
		dvbfe_set_tone_data_burst(fe, DVBFE_SEC_MINI_B);
		break;
	default:
		break;
	}

	if (sat_pos != DISEQC_SWITCH_UNCHANGED)
		usleep(15000);

	switch(oscillator) {
	case DISEQC_OSCILLATOR_LOW:
		dvbfe_set_22k_tone(fe, DVBFE_SEC_TONE_OFF);
		break;
	case DISEQC_OSCILLATOR_HIGH:
		dvbfe_set_22k_tone(fe, DVBFE_SEC_TONE_ON);
		break;
	default:
		break;
	}

	return 0;
}

int dvbsec_diseqc_set_reset(struct dvbfe_handle *fe,
			   enum dvbsec_diseqc_address address,
			   enum dvbsec_diseqc_reset state)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x00 };

	if (state == DISEQC_RESET_CLEAR)
		data[2] = 0x01;

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_set_power(struct dvbfe_handle *fe,
			   enum dvbsec_diseqc_address address,
			   enum dvbsec_diseqc_power state)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x02 };

	if (state == DISEQC_POWER_ON)
		data[2] = 0x03;

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_set_listen(struct dvbfe_handle *fe,
			    enum dvbsec_diseqc_address address,
			    enum dvbsec_diseqc_listen state)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x30 };

	if (state == DISEQC_LISTEN_AWAKE)
		data[2] = 0x31;

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_set_committed_switches(struct dvbfe_handle *fe,
					enum dvbsec_diseqc_address address,
					enum dvbsec_diseqc_oscillator oscillator,
					enum dvbsec_diseqc_polarization polarization,
					enum dvbsec_diseqc_switch sat_pos,
					enum dvbsec_diseqc_switch switch_option)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x38, 0x00 };

	switch(oscillator) {
	case DISEQC_OSCILLATOR_LOW:
		data[3] |= 0x10;
		break;
	case DISEQC_OSCILLATOR_HIGH:
		data[3] |= 0x11;
		break;
	case DISEQC_OSCILLATOR_UNCHANGED:
		break;
	}
	switch(polarization) {
	case DISEQC_POLARIZATION_V:
	case DISEQC_POLARIZATION_R:
		data[3] |= 0x20;
		break;
	case DISEQC_POLARIZATION_H:
	case DISEQC_POLARIZATION_L:
		data[3] |= 0x22;
		break;
	default:
		break;
	}
	switch(sat_pos) {
	case DISEQC_SWITCH_A:
		data[3] |= 0x40;
		break;
	case DISEQC_SWITCH_B:
		data[3] |= 0x44;
		break;
	case DISEQC_SWITCH_UNCHANGED:
		break;
	}
	switch(switch_option) {
	case DISEQC_SWITCH_A:
		data[3] |= 0x80;
		break;
	case DISEQC_SWITCH_B:
		data[3] |= 0x88;
		break;
	case DISEQC_SWITCH_UNCHANGED:
		break;
	}

	if (data[3] == 0)
		return 0;

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_set_uncommitted_switches(struct dvbfe_handle *fe,
					  enum dvbsec_diseqc_address address,
					  enum dvbsec_diseqc_switch s1,
					  enum dvbsec_diseqc_switch s2,
					  enum dvbsec_diseqc_switch s3,
					  enum dvbsec_diseqc_switch s4)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x39, 0x00 };

	switch(s1) {
	case DISEQC_SWITCH_A:
		data[3] |= 0x10;
		break;
	case DISEQC_SWITCH_B:
		data[3] |= 0x11;
		break;
	case DISEQC_SWITCH_UNCHANGED:
		break;
	}
	switch(s2) {
	case DISEQC_SWITCH_A:
		data[3] |= 0x20;
		break;
	case DISEQC_SWITCH_B:
		data[3] |= 0x22;
		break;
	case DISEQC_SWITCH_UNCHANGED:
		break;
	}
	switch(s3) {
	case DISEQC_SWITCH_A:
		data[3] |= 0x40;
		break;
	case DISEQC_SWITCH_B:
		data[3] |= 0x44;
		break;
	case DISEQC_SWITCH_UNCHANGED:
		break;
	}
	switch(s4) {
	case DISEQC_SWITCH_A:
		data[3] |= 0x80;
		break;
	case DISEQC_SWITCH_B:
		data[3] |= 0x88;
		break;
	case DISEQC_SWITCH_UNCHANGED:
		break;
	}

	if (data[3] == 0)
		return 0;

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_set_analog_value(struct dvbfe_handle *fe,
				  enum dvbsec_diseqc_address address,
				  enum dvbsec_diseqc_analog_id id,
				  uint8_t value)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x48, value };

	if (id == DISEQC_ANALOG_ID_A1)
		data[2] = 0x49;

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_set_frequency(struct dvbfe_handle *fe,
			       enum dvbsec_diseqc_address address,
			       uint32_t frequency)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x58, 0x00, 0x00, 0x00 };
	int len = 5;

	uint32_t bcdval = 0;
	int i;
	for(i=0; i<=24;i+=4) {
		bcdval |= ((frequency % 10) << i);
		frequency /= 10;
	}

	data[3] = bcdval >> 16;
	data[4] = bcdval >> 8;
	if (bcdval & 0xff) {
		data[5] = bcdval;
		len++;
	}

	return dvbfe_do_diseqc_command(fe, data, len);
}

int dvbsec_diseqc_set_channel(struct dvbfe_handle *fe,
			     enum dvbsec_diseqc_address address,
			     uint16_t channel)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x59, 0x00, 0x00};

	data[3] = channel >> 8;
	data[4] = channel;

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_halt_satpos(struct dvbfe_handle *fe,
			     enum dvbsec_diseqc_address address)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x60};

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_disable_satpos_limits(struct dvbfe_handle *fe,
				       enum dvbsec_diseqc_address address)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x63};

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_set_satpos_limit(struct dvbfe_handle *fe,
				  enum dvbsec_diseqc_address address,
				  enum dvbsec_diseqc_direction direction)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x66};

	if (direction == DISEQC_DIRECTION_WEST)
		data[2] = 0x67;

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_drive_satpos_motor(struct dvbfe_handle *fe,
				    enum dvbsec_diseqc_address address,
				    enum dvbsec_diseqc_direction direction,
				    enum dvbsec_diseqc_drive_mode mode,
				    uint8_t value)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x68, 0x00};

	if (direction == DISEQC_DIRECTION_WEST)
		data[2] = 0x69;

	switch(mode) {
	case DISEQC_DRIVE_MODE_STEPS:
		data[3] = (value & 0x7f) | 0x80;
		break;
	case DISEQC_DRIVE_MODE_TIMEOUT:
		data[3] = value & 0x7f;
		break;
	}

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_store_satpos_preset(struct dvbfe_handle *fe,
				     enum dvbsec_diseqc_address address,
				     uint8_t id)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x6A, id};

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_goto_satpos_preset(struct dvbfe_handle *fe,
				    enum dvbsec_diseqc_address address,
				    uint8_t id)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x6B, id};

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

int dvbsec_diseqc_recalculate_satpos_positions(struct dvbfe_handle *fe,
					      enum dvbsec_diseqc_address address,
					      int val1,
					      int val2)
{
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x6F, 0x00, 0x00};
	int len = 3;

	if (val1 != -1) {
		data[3] = val1;
		len++;
	}
	if (val2 != -1) {
		data[4] = val2;
		len = 5;
	}

	return dvbfe_do_diseqc_command(fe, data, len);
}

int dvbsec_diseqc_goto_rotator_bearing(struct dvbfe_handle *fe,
				      enum dvbsec_diseqc_address address,
				      float angle)
{
	int integer = (int) angle;
	uint8_t data[] = { DISEQC_FRAMING_MASTER_NOREPLY, address, 0x6e, 0x00, 0x00};

	// transform the fraction into the correct representation
	int fraction = (int) (((angle - integer) * 16.0) + 0.9) & 0x0f;
	switch(fraction) {
	case 1:
	case 4:
	case 7:
	case 9:
	case 12:
	case 15:
		fraction--;
		break;
	}

	// generate the command
	if (integer < 0.0) {
		data[3] = 0xD0;  // West is a negative angle value
	} else if (integer >= 0.0) {
		data[3] = 0xE0;  // East is a positive angle value
	}
	integer = abs(integer);
	data[3] |= ((integer / 16) & 0x0f);
	integer = integer % 16;
	data[4] |= ((integer & 0x0f) << 4) | fraction;

	return dvbfe_do_diseqc_command(fe, data, sizeof(data));
}

static int skipwhite(char **line, char *end)
{
	while(**line) {
		if (end && (*line >= end))
			return -1;
		if (!isspace(**line))
			return 0;
		(*line)++;
	}

	return -1;
}

static int getstringupto(char **line, char *end, char *matches, char **ptrdest, int *ptrlen)
{
	char *start = *line;

	while(**line) {
		if (end && (*line >= end))
			break;
		if (strchr(matches, **line)) {
			*ptrdest = start;
			*ptrlen = *line - start;
			return 0;
		}
		(*line)++;
	}

	*ptrdest = start;
	*ptrlen = *line - start;
	return 0;
}

static int parsefunction(char **line,
			 char **nameptr, int *namelen,
			 char **argsptr, int *argslen)
{
	if (skipwhite(line, NULL))
		return -1;

	if (getstringupto(line, NULL, "(", nameptr, namelen))
		return -1;
	if ((*line) == 0)
		return -1;
	(*line)++; // skip the '('
	if (getstringupto(line, NULL, ")", argsptr, argslen))
		return -1;
	if ((*line) == 0)
		return -1;
	(*line)++; // skip the ')'

	return 0;
}

static int parseintarg(char **args, char *argsend, int *result)
{
	char tmp[32];
	char *arg;
	int arglen;

	// skip whitespace
	if (skipwhite(args, argsend))
		return -1;

	// get the arg
	if (getstringupto(args, argsend, ",", &arg, &arglen))
		return -1;
	if ((**args) == ',')
		(*args)++; // skip the ',' if present
	if (arglen > 31)
		arglen = 31;
	strncpy(tmp, arg, arglen);
	tmp[arglen] = 0;

	if (sscanf(tmp, "%i", result) != 1)
		return -1;

	return 0;
}

static int parsechararg(char **args, char *argsend, int *result)
{
	char *arg;
	int arglen;

	// skip whitespace
	if (skipwhite(args, argsend))
		return -1;

	// get the arg
	if (getstringupto(args, argsend, ",", &arg, &arglen))
		return -1;
	if ((**args) == ',')
		(*args)++; // skip the ',' if present
	if (arglen > 0)
		*result = arg[0];

	return 0;
}

static int parsefloatarg(char **args, char *argsend, float *result)
{
	char tmp[32];
	char *arg;
	int arglen;

	// skip whitespace
	if (skipwhite(args, argsend))
		return -1;

	// get the arg
	if (getstringupto(args, argsend, ",", &arg, &arglen))
		return -1;
	if ((**args) == ',')
		(*args)++; // skip the ',' if present
	if (arglen > 31)
		arglen = 31;
	strncpy(tmp, arg, arglen);
	arg[arglen] = 0;

	if (sscanf(tmp, "%f", result) != 1)
		return -1;

	return 0;
}

static enum dvbsec_diseqc_switch parse_switch(int c)
{
	switch(toupper(c)) {
	case 'A':
		return DISEQC_SWITCH_A;
	case 'B':
		return DISEQC_SWITCH_B;
	default:
		return DISEQC_SWITCH_UNCHANGED;
	}
}

int dvbsec_command(struct dvbfe_handle *fe, char *command)
{
	char *name;
	char *args;
	int namelen;
	int argslen;
	int address;
	int iarg;
	int iarg2;
	int iarg3;
	int iarg4;
	float farg;

	while(!parsefunction(&command, &name, &namelen, &args, &argslen)) {
		char *argsend = args+argslen;

		if (!strncasecmp(name, "tone", namelen)) {
			if (parsechararg(&args, argsend, &iarg))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("tone: %c\n", iarg);
#else
			if (toupper(iarg) == 'B') {
				dvbfe_set_22k_tone(fe, DVBFE_SEC_TONE_ON);
			} else {
				dvbfe_set_22k_tone(fe, DVBFE_SEC_TONE_OFF);
			}
#endif
		} else if (!strncasecmp(name, "voltage", namelen)) {
			if (parseintarg(&args, argsend, &iarg))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("voltage: %i\n", iarg);
#else
			switch(iarg) {
			case 0:
				dvbfe_set_voltage(fe, DVBFE_SEC_VOLTAGE_OFF);
				break;
			case 13:
				dvbfe_set_voltage(fe, DVBFE_SEC_VOLTAGE_13);
				break;
			case 18:
				dvbfe_set_voltage(fe, DVBFE_SEC_VOLTAGE_18);
				break;
			default:
				return -1;
			}
#endif
		} else if (!strncasecmp(name, "toneburst", namelen)) {
			if (parsechararg(&args, argsend, &iarg))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("toneburst: %c\n", iarg);
#else
			if (toupper(iarg) == 'B') {
				dvbfe_set_tone_data_burst(fe, DVBFE_SEC_MINI_B);
			} else {
				dvbfe_set_tone_data_burst(fe, DVBFE_SEC_MINI_A);
			}
#endif
		} else if (!strncasecmp(name, "highvoltage", namelen)) {
			if (parseintarg(&args, argsend, &iarg))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("highvoltage: %i\n", iarg);
#else
			dvbfe_set_high_lnb_voltage(fe, iarg ? 1 : 0);
#endif
		} else if (!strncasecmp(name, "dishnetworks", namelen)) {
			if (parseintarg(&args, argsend, &iarg))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("dishnetworks: %i\n", iarg);
#else
			dvbfe_do_dishnetworks_legacy_command(fe, iarg);
#endif
		} else if (!strncasecmp(name, "wait", namelen)) {
			if (parseintarg(&args, argsend, &iarg))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("wait: %i\n", iarg);
#else
			if (iarg)
				usleep(iarg * 1000);
#endif
		} else if (!strncasecmp(name, "Dreset", namelen)) {
			if (parseintarg(&args, argsend, &address))
				return -1;
			if (parseintarg(&args, argsend, &iarg))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("Dreset: %i %i\n", address, iarg);
#else
			if (iarg) {
				dvbsec_diseqc_set_reset(fe, address, DISEQC_RESET);
			} else {
				dvbsec_diseqc_set_reset(fe, address, DISEQC_RESET_CLEAR);
			}
#endif
		} else if (!strncasecmp(name, "Dpower", namelen)) {
			if (parseintarg(&args, argsend, &address))
				return -1;
			if (parseintarg(&args, argsend, &iarg))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("Dpower: %i %i\n", address, iarg);
#else
			if (iarg) {
				dvbsec_diseqc_set_power(fe, address, DISEQC_POWER_ON);
			} else {
				dvbsec_diseqc_set_power(fe, address, DISEQC_POWER_OFF);
			}
#endif
		} else if (!strncasecmp(name, "Dcommitted", namelen)) {
			if (parseintarg(&args, argsend, &address))
				return -1;
			if (parsechararg(&args, argsend, &iarg))
				return -1;
			if (parsechararg(&args, argsend, &iarg2))
				return -1;
			if (parsechararg(&args, argsend, &iarg3))
				return -1;
			if (parsechararg(&args, argsend, &iarg4))
				return -1;

			enum dvbsec_diseqc_oscillator oscillator;
			switch(toupper(iarg)) {
			case 'H':
				oscillator = DISEQC_OSCILLATOR_HIGH;
				break;
			case 'L':
				oscillator = DISEQC_OSCILLATOR_LOW;
				break;
			default:
				oscillator = DISEQC_OSCILLATOR_UNCHANGED;
				break;
			}

			int polarization = -1;
			switch(toupper(iarg2)) {
			case 'H':
				polarization = DISEQC_POLARIZATION_H;
				break;
			case 'V':
				polarization = DISEQC_POLARIZATION_V;
				break;
			case 'L':
				polarization = DISEQC_POLARIZATION_L;
				break;
			case 'R':
				polarization = DISEQC_POLARIZATION_R;
				break;
			default:
				polarization = -1;
				break;
			}

#ifdef TEST_SEC_COMMAND
			printf("Dcommitted: %i %i %i %i %i\n", address,
			       oscillator,
			       polarization,
			       parse_switch(iarg3),
			       parse_switch(iarg4));
#else
			dvbsec_diseqc_set_committed_switches(fe, address,
							    oscillator,
							    polarization,
							    parse_switch(iarg3),
							    parse_switch(iarg4));
#endif
		} else if (!strncasecmp(name, "Duncommitted", namelen)) {
			if (parsechararg(&args, argsend, &address))
				return -1;
			if (parsechararg(&args, argsend, &iarg))
				return -1;
			if (parsechararg(&args, argsend, &iarg2))
				return -1;
			if (parsechararg(&args, argsend, &iarg3))
				return -1;
			if (parsechararg(&args, argsend, &iarg4))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("Duncommitted: %i %i %i %i %i\n", address,
			       parse_switch(iarg),
			       parse_switch(iarg2),
			       parse_switch(iarg3),
			       parse_switch(iarg4));
#else
			dvbsec_diseqc_set_uncommitted_switches(fe, address,
					parse_switch(iarg),
					parse_switch(iarg2),
					parse_switch(iarg3),
					parse_switch(iarg4));
#endif
		} else if (!strncasecmp(name, "Dfrequency", namelen)) {
			if (parseintarg(&args, argsend, &address))
				return -1;
			if (parseintarg(&args, argsend, &iarg))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("Dfrequency: %i %i\n", address, iarg);
#else
			dvbsec_diseqc_set_frequency(fe, address, iarg);
#endif
		} else if (!strncasecmp(name, "Dchannel", namelen)) {
			if (parseintarg(&args, argsend, &address))
				return -1;
			if (parseintarg(&args, argsend, &iarg))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("Dchannel: %i %i\n", address, iarg);
#else
			dvbsec_diseqc_set_channel(fe, address, iarg);
#endif
		} else if (!strncasecmp(name, "Dgotopreset", namelen)) {
			if (parseintarg(&args, argsend, &address))
				return -1;
			if (parseintarg(&args, argsend, &iarg))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("Dgotopreset: %i %i\n", address, iarg);
#else
			dvbsec_diseqc_goto_satpos_preset(fe, address, iarg);
#endif
		} else if (!strncasecmp(name, "Dgotobearing", namelen)) {
			if (parseintarg(&args, argsend, &address))
				return -1;
			if (parsefloatarg(&args, argsend, &farg))
				return -1;

#ifdef TEST_SEC_COMMAND
			printf("Dgotobearing: %i %f\n", address, farg);
#else
			dvbsec_diseqc_goto_rotator_bearing(fe, address, farg);
#endif
		} else {
			return -1;
		}
	}

	return 0;
}
