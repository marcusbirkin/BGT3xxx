/*
 * dvbcfg - support for linuxtv configuration files
 * zap channel file support
 *
 * Copyright (C) 2006 Christoph Pfister <christophpfister@gmail.com>
 * Copyright (C) 2005 Andrew de Quincey <adq_dvb@lidskialf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define _GNU_SOURCE

#include <malloc.h>
#include <string.h>
#include <ctype.h>

#include "dvbcfg_zapchannel.h"
#include "dvbcfg_common.h"

static const struct dvbcfg_setting dvbcfg_inversion_list[] = {
	{ "INVERSION_ON",   DVBFE_INVERSION_ON   },
	{ "INVERSION_OFF",  DVBFE_INVERSION_OFF  },
	{ "INVERSION_AUTO", DVBFE_INVERSION_AUTO },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_fec_list[] = {
	{ "FEC_1_2",  DVBFE_FEC_1_2  },
	{ "FEC_2_3",  DVBFE_FEC_2_3  },
	{ "FEC_3_4",  DVBFE_FEC_3_4  },
	{ "FEC_4_5",  DVBFE_FEC_4_5  },
	{ "FEC_5_6",  DVBFE_FEC_5_6  },
	{ "FEC_6_7",  DVBFE_FEC_6_7  },
	{ "FEC_7_8",  DVBFE_FEC_7_8  },
	{ "FEC_8_9",  DVBFE_FEC_8_9  },
	{ "FEC_AUTO", DVBFE_FEC_AUTO },
	{ "FEC_NONE", DVBFE_FEC_NONE },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_dvbc_modulation_list[] = {
	{ "QAM_16",   DVBFE_DVBC_MOD_QAM_16  },
	{ "QAM_32",   DVBFE_DVBC_MOD_QAM_32  },
	{ "QAM_64",   DVBFE_DVBC_MOD_QAM_64  },
	{ "QAM_128",  DVBFE_DVBC_MOD_QAM_128 },
	{ "QAM_256",  DVBFE_DVBC_MOD_QAM_256 },
	{ "QAM_AUTO", DVBFE_DVBC_MOD_AUTO    },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_bandwidth_list[] = {
	{ "BANDWIDTH_6_MHZ", DVBFE_DVBT_BANDWIDTH_6_MHZ },
	{ "BANDWIDTH_7_MHZ", DVBFE_DVBT_BANDWIDTH_7_MHZ },
	{ "BANDWIDTH_8_MHZ", DVBFE_DVBT_BANDWIDTH_8_MHZ },
	{ "BANDWIDTH_AUTO",  DVBFE_DVBT_BANDWIDTH_AUTO  },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_constellation_list[] = {
	{ "QAM_16",   DVBFE_DVBT_CONST_QAM_16  },
	{ "QAM_32",   DVBFE_DVBT_CONST_QAM_32  },
	{ "QAM_64",   DVBFE_DVBT_CONST_QAM_64  },
	{ "QAM_128",  DVBFE_DVBT_CONST_QAM_128 },
	{ "QAM_256",  DVBFE_DVBT_CONST_QAM_256 },
	{ "QPSK",     DVBFE_DVBT_CONST_QPSK    },
	{ "QAM_AUTO", DVBFE_DVBT_CONST_AUTO    },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_transmission_mode_list[] = {
	{ "TRANSMISSION_MODE_2K",   DVBFE_DVBT_TRANSMISSION_MODE_2K   },
	{ "TRANSMISSION_MODE_8K",   DVBFE_DVBT_TRANSMISSION_MODE_8K   },
	{ "TRANSMISSION_MODE_AUTO", DVBFE_DVBT_TRANSMISSION_MODE_AUTO },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_guard_interval_list[] = {
	{ "GUARD_INTERVAL_1_32", DVBFE_DVBT_GUARD_INTERVAL_1_32 },
	{ "GUARD_INTERVAL_1_16", DVBFE_DVBT_GUARD_INTERVAL_1_16 },
	{ "GUARD_INTERVAL_1_8",  DVBFE_DVBT_GUARD_INTERVAL_1_8  },
	{ "GUARD_INTERVAL_1_4",  DVBFE_DVBT_GUARD_INTERVAL_1_4  },
	{ "GUARD_INTERVAL_AUTO", DVBFE_DVBT_GUARD_INTERVAL_AUTO },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_hierarchy_list[] = {
	{ "HIERARCHY_1",    DVBFE_DVBT_HIERARCHY_1    },
	{ "HIERARCHY_2",    DVBFE_DVBT_HIERARCHY_2    },
	{ "HIERARCHY_4",    DVBFE_DVBT_HIERARCHY_4    },
	{ "HIERARCHY_AUTO", DVBFE_DVBT_HIERARCHY_AUTO },
	{ "HIERARCHY_NONE", DVBFE_DVBT_HIERARCHY_NONE },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_atsc_modulation_list[] = {
	{ "8VSB",    DVBFE_ATSC_MOD_VSB_8   },
	{ "16VSB",   DVBFE_ATSC_MOD_VSB_16  },
	{ "QAM_64",  DVBFE_ATSC_MOD_QAM_64  },
	{ "QAM_256", DVBFE_ATSC_MOD_QAM_256 },
	{ NULL, 0 }
};

int dvbcfg_zapchannel_parse(FILE *file, dvbcfg_zapcallback callback, void *private_data)
{
	char *line_buf = NULL;
	size_t line_size = 0;
	int line_len = 0;
	int ret_val = 0;

	while ((line_len = getline(&line_buf, &line_size, file)) > 0) {
		char *line_tmp = line_buf;
		char *line_pos = line_buf;
		struct dvbcfg_zapchannel tmp;

		/* remove newline and comments (started with hashes) */
		while ((*line_tmp != '\0') && (*line_tmp != '\n') && (*line_tmp != '#'))
			line_tmp++;
		*line_tmp = '\0';

		/* parse name */
		dvbcfg_parse_string(&line_pos, ":", tmp.name, sizeof(tmp.name));
		if (!line_pos)
			continue;

		/* parse frequency */
		tmp.fe_params.frequency = dvbcfg_parse_int(&line_pos, ":");
		if (!line_pos)
			continue;

		/* try to determine frontend type */
		if (strstr(line_pos, ":FEC_")) {
			if (strstr(line_pos, ":HIERARCHY_"))
				tmp.fe_type = DVBFE_TYPE_DVBT;
			else
				tmp.fe_type = DVBFE_TYPE_DVBC;
		} else {
			if (strstr(line_pos, "VSB:") || strstr(line_pos, "QAM_"))
				tmp.fe_type = DVBFE_TYPE_ATSC;
			else
				tmp.fe_type = DVBFE_TYPE_DVBS;
		}

		/* parse frontend specific settings */
		switch (tmp.fe_type) {
		case DVBFE_TYPE_ATSC:
			/* inversion */
			tmp.fe_params.inversion = DVBFE_INVERSION_AUTO;

			/* modulation */
			tmp.fe_params.u.atsc.modulation =
				dvbcfg_parse_setting(&line_pos, ":", dvbcfg_atsc_modulation_list);
			if (!line_pos)
				continue;

			break;

		case DVBFE_TYPE_DVBC:
			/* inversion */
			tmp.fe_params.inversion =
				dvbcfg_parse_setting(&line_pos, ":", dvbcfg_inversion_list);
			if (!line_pos)
				continue;

			/* symbol rate */
			tmp.fe_params.u.dvbc.symbol_rate = dvbcfg_parse_int(&line_pos, ":");
			if (!line_pos)
				continue;

			/* fec */
			tmp.fe_params.u.dvbc.fec_inner =
				dvbcfg_parse_setting(&line_pos, ":", dvbcfg_fec_list);
			if (!line_pos)
				continue;

			/* modulation */
			tmp.fe_params.u.dvbc.modulation =
				dvbcfg_parse_setting(&line_pos, ":", dvbcfg_dvbc_modulation_list);
			if (!line_pos)
				continue;

			break;

		case DVBFE_TYPE_DVBS:
			/* adjust frequency */
			tmp.fe_params.frequency *= 1000;

			/* inversion */
			tmp.fe_params.inversion = DVBFE_INVERSION_AUTO;

			/* fec */
			tmp.fe_params.u.dvbs.fec_inner = DVBFE_FEC_AUTO;

			/* polarization */
			tmp.polarization = tolower(dvbcfg_parse_char(&line_pos, ":"));
			if (!line_pos)
				continue;
			if ((tmp.polarization != 'h') &&
			    (tmp.polarization != 'v') &&
			    (tmp.polarization != 'l') &&
			    (tmp.polarization != 'r'))
				continue;

			/* satellite switch position */
			tmp.diseqc_switch = dvbcfg_parse_int(&line_pos, ":");
			if (!line_pos)
				continue;

			/* symbol rate */
			tmp.fe_params.u.dvbs.symbol_rate =
				dvbcfg_parse_int(&line_pos, ":") * 1000;
			if (!line_pos)
				continue;

			break;

		case DVBFE_TYPE_DVBT:
			/* inversion */
			tmp.fe_params.inversion =
				dvbcfg_parse_setting(&line_pos, ":", dvbcfg_inversion_list);
			if (!line_pos)
				continue;

			/* bandwidth */
			tmp.fe_params.u.dvbt.bandwidth =
				dvbcfg_parse_setting(&line_pos, ":", dvbcfg_bandwidth_list);
			if (!line_pos)
				continue;

			/* fec hp */
			tmp.fe_params.u.dvbt.code_rate_HP =
				dvbcfg_parse_setting(&line_pos, ":", dvbcfg_fec_list);
			if (!line_pos)
				continue;

			/* fec lp */
			tmp.fe_params.u.dvbt.code_rate_LP =
				dvbcfg_parse_setting(&line_pos, ":", dvbcfg_fec_list);
			if (!line_pos)
				continue;

			/* constellation */
			tmp.fe_params.u.dvbt.constellation =
				dvbcfg_parse_setting(&line_pos, ":", dvbcfg_constellation_list);
			if (!line_pos)
				continue;

			/* transmission mode */
			tmp.fe_params.u.dvbt.transmission_mode =
				dvbcfg_parse_setting(&line_pos, ":", dvbcfg_transmission_mode_list);
			if (!line_pos)
				continue;

			/* guard interval */
			tmp.fe_params.u.dvbt.guard_interval =
				dvbcfg_parse_setting(&line_pos, ":", dvbcfg_guard_interval_list);
			if (!line_pos)
				continue;

			/* hierarchy */
			tmp.fe_params.u.dvbt.hierarchy_information =
				dvbcfg_parse_setting(&line_pos, ":", dvbcfg_hierarchy_list);
			if (!line_pos)
				continue;

			break;
		}

		/* parse video and audio pids and service id */
		tmp.video_pid = dvbcfg_parse_int(&line_pos, ":");
		if (!line_pos)
			continue;
		tmp.audio_pid = dvbcfg_parse_int(&line_pos, ":");
		if (!line_pos)
			continue;
		tmp.service_id = dvbcfg_parse_int(&line_pos, ":");
		if (!line_pos) /* old files don't have a service id */
			tmp.service_id = 0;

		/* invoke callback */
		if ((ret_val = callback(&tmp, private_data)) != 0) {
			if (ret_val < 0)
				ret_val = 0;
			break;
		}
	}

	if (line_buf)
		free(line_buf);

	return ret_val;
}

int dvbcfg_zapchannel_save(FILE *file, dvbcfg_zapcallback callback, void *private_data)
{
	int ret_val = 0;
	struct dvbcfg_zapchannel tmp;

	while ((ret_val = callback(&tmp, private_data)) == 0) {
		/* name */
		if ((ret_val = fprintf(file, "%s:", tmp.name)) < 0)
			return ret_val;

		/* frontend specific settings */
		switch (tmp.fe_type) {
		case DVBFE_TYPE_ATSC:
			if ((ret_val = fprintf(file, "%i:%s:",
			    tmp.fe_params.frequency,
			    dvbcfg_lookup_setting(tmp.fe_params.u.atsc.modulation,
						  dvbcfg_atsc_modulation_list))) < 0)
				return ret_val;

			break;

		case DVBFE_TYPE_DVBC:
			if ((ret_val = fprintf(file, "%i:%s:%i:%s:%s:",
			    tmp.fe_params.frequency,
			    dvbcfg_lookup_setting(tmp.fe_params.inversion,
						  dvbcfg_inversion_list),
			    tmp.fe_params.u.dvbc.symbol_rate,
			    dvbcfg_lookup_setting(tmp.fe_params.u.dvbc.fec_inner,
						  dvbcfg_fec_list),
			    dvbcfg_lookup_setting(tmp.fe_params.u.dvbc.modulation,
						  dvbcfg_dvbc_modulation_list))) < 0)
				return ret_val;

			break;

		case DVBFE_TYPE_DVBS:
			if ((ret_val = fprintf(file, "%i:%c:%i:%i:",
			    tmp.fe_params.frequency / 1000,
			    tolower(tmp.polarization),
			    tmp.diseqc_switch,
			    tmp.fe_params.u.dvbs.symbol_rate / 1000)) < 0)
				return ret_val;

			break;
		case DVBFE_TYPE_DVBT:
			if ((ret_val = fprintf(file, "%i:%s:%s:%s:%s:%s:%s:%s:%s:",
			    tmp.fe_params.frequency,
			    dvbcfg_lookup_setting(tmp.fe_params.inversion,
						  dvbcfg_inversion_list),
			    dvbcfg_lookup_setting(tmp.fe_params.u.dvbt.bandwidth,
						  dvbcfg_bandwidth_list),
			    dvbcfg_lookup_setting(tmp.fe_params.u.dvbt.code_rate_HP,
						  dvbcfg_fec_list),
			    dvbcfg_lookup_setting(tmp.fe_params.u.dvbt.code_rate_LP,
						  dvbcfg_fec_list),
			    dvbcfg_lookup_setting(tmp.fe_params.u.dvbt.constellation,
						  dvbcfg_constellation_list),
			    dvbcfg_lookup_setting(tmp.fe_params.u.dvbt.transmission_mode,
						  dvbcfg_transmission_mode_list),
			    dvbcfg_lookup_setting(tmp.fe_params.u.dvbt.guard_interval,
						  dvbcfg_guard_interval_list),
			    dvbcfg_lookup_setting(tmp.fe_params.u.dvbt.hierarchy_information,
						  dvbcfg_hierarchy_list))) < 0)
				return ret_val;

			break;
		}

		/* video and audio pids and service id */
		if ((ret_val = fprintf(file, "%i:%i:%i\n",
		    tmp.video_pid, tmp.audio_pid, tmp.service_id)) < 0)
			return ret_val;

	}

	if (ret_val < 0)
		ret_val = 0;

	return ret_val;
}
