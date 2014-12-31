/*
 * dvbcfg - support for linuxtv configuration files
 * scan channel file support
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
#include <ctype.h>

#include "dvbcfg_scanfile.h"
#include "dvbcfg_common.h"

static const struct dvbcfg_setting dvbcfg_fec_list[] = {
	{ "1/2",  DVBFE_FEC_1_2  },
	{ "2/3",  DVBFE_FEC_2_3  },
	{ "3/4",  DVBFE_FEC_3_4  },
	{ "4/5",  DVBFE_FEC_4_5  },
	{ "5/6",  DVBFE_FEC_5_6  },
	{ "6/7",  DVBFE_FEC_6_7  },
	{ "7/8",  DVBFE_FEC_7_8  },
	{ "8/9",  DVBFE_FEC_8_9  },
	{ "AUTO", DVBFE_FEC_AUTO },
	{ "NONE", DVBFE_FEC_NONE },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_dvbc_modulation_list[] = {
	{ "QAM16",   DVBFE_DVBC_MOD_QAM_16  },
	{ "QAM32",   DVBFE_DVBC_MOD_QAM_32  },
	{ "QAM64",   DVBFE_DVBC_MOD_QAM_64  },
	{ "QAM128",  DVBFE_DVBC_MOD_QAM_128 },
	{ "QAM256",  DVBFE_DVBC_MOD_QAM_256 },
	{ "AUTO", DVBFE_DVBC_MOD_AUTO    },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_bandwidth_list[] = {
	{ "6MHz", DVBFE_DVBT_BANDWIDTH_6_MHZ },
	{ "7MHz", DVBFE_DVBT_BANDWIDTH_7_MHZ },
	{ "8MHz", DVBFE_DVBT_BANDWIDTH_8_MHZ },
	{ "AUTO",  DVBFE_DVBT_BANDWIDTH_AUTO  },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_constellation_list[] = {
	{ "QAM16",   DVBFE_DVBT_CONST_QAM_16  },
	{ "QAM32",   DVBFE_DVBT_CONST_QAM_32  },
	{ "QAM64",   DVBFE_DVBT_CONST_QAM_64  },
	{ "QAM128",  DVBFE_DVBT_CONST_QAM_128 },
	{ "QAM256",  DVBFE_DVBT_CONST_QAM_256 },
	{ "QPSK",     DVBFE_DVBT_CONST_QPSK    },
	{ "AUTO", DVBFE_DVBT_CONST_AUTO    },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_transmission_mode_list[] = {
	{ "2k",   DVBFE_DVBT_TRANSMISSION_MODE_2K   },
	{ "8k",   DVBFE_DVBT_TRANSMISSION_MODE_8K   },
	{ "AUTO", DVBFE_DVBT_TRANSMISSION_MODE_AUTO },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_guard_interval_list[] = {
	{ "1/32", DVBFE_DVBT_GUARD_INTERVAL_1_32 },
	{ "1/16", DVBFE_DVBT_GUARD_INTERVAL_1_16 },
	{ "1/8",  DVBFE_DVBT_GUARD_INTERVAL_1_8  },
	{ "1/4",  DVBFE_DVBT_GUARD_INTERVAL_1_4  },
	{ "AUTO", DVBFE_DVBT_GUARD_INTERVAL_AUTO },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_hierarchy_list[] = {
	{ "1",    DVBFE_DVBT_HIERARCHY_1    },
	{ "2",    DVBFE_DVBT_HIERARCHY_2    },
	{ "4",    DVBFE_DVBT_HIERARCHY_4    },
	{ "AUTO", DVBFE_DVBT_HIERARCHY_AUTO },
	{ "NONE", DVBFE_DVBT_HIERARCHY_NONE },
	{ NULL, 0 }
};

static const struct dvbcfg_setting dvbcfg_atsc_modulation_list[] = {
	{ "8VSB",    DVBFE_ATSC_MOD_VSB_8   },
	{ "16VSB",   DVBFE_ATSC_MOD_VSB_16  },
	{ "QAM64",  DVBFE_ATSC_MOD_QAM_64  },
	{ "QAM256", DVBFE_ATSC_MOD_QAM_256 },
	{ NULL, 0 }
};

int dvbcfg_scanfile_parse(FILE *file, dvbcfg_scancallback callback, void *private_data)
{
	char *line_buf = NULL;
	size_t line_size = 0;
	int line_len = 0;
	int ret_val = 0;

	while ((line_len = getline(&line_buf, &line_size, file)) > 0) {
		char *line_tmp = line_buf;
		char *line_pos = line_buf;
		struct dvbcfg_scanfile tmp;

		/* remove newline and comments (started with hashes) */
		while ((*line_tmp != '\0') && (*line_tmp != '\n') && (*line_tmp != '#'))
			line_tmp++;
		*line_tmp = '\0';

		/* always use inversion auto */
		tmp.fe_params.inversion = DVBFE_INVERSION_AUTO;

		/* parse frontend type */
		switch(dvbcfg_parse_char(&line_pos, " ")) {
		case 'T':
			tmp.fe_type = DVBFE_TYPE_DVBT;
			break;
		case 'C':
			tmp.fe_type = DVBFE_TYPE_DVBC;
			break;
		case 'S':
			tmp.fe_type = DVBFE_TYPE_DVBS;
			break;
		case 'A':
			tmp.fe_type = DVBFE_TYPE_ATSC;
			break;
		default:
			continue;
		}

		/* parse frontend specific settings */
		switch (tmp.fe_type) {
		case DVBFE_TYPE_ATSC:

			/* parse frequency */
			tmp.fe_params.frequency = dvbcfg_parse_int(&line_pos, " ");
			if (!line_pos)
				continue;

			/* modulation */
			tmp.fe_params.u.atsc.modulation =
				dvbcfg_parse_setting(&line_pos, " ", dvbcfg_atsc_modulation_list);
			if (!line_pos)
				continue;

			break;

		case DVBFE_TYPE_DVBC:

			/* parse frequency */
			tmp.fe_params.frequency = dvbcfg_parse_int(&line_pos, " ");
			if (!line_pos)
				continue;

			/* symbol rate */
			tmp.fe_params.u.dvbc.symbol_rate = dvbcfg_parse_int(&line_pos, " ");
			if (!line_pos)
				continue;

			/* fec */
			tmp.fe_params.u.dvbc.fec_inner =
				dvbcfg_parse_setting(&line_pos, " ", dvbcfg_fec_list);
			if (!line_pos)
				continue;

			/* modulation */
			tmp.fe_params.u.dvbc.modulation =
				dvbcfg_parse_setting(&line_pos, " ", dvbcfg_dvbc_modulation_list);
			if (!line_pos)
				continue;

			break;

		case DVBFE_TYPE_DVBS:

			/* parse frequency */
			tmp.fe_params.frequency = dvbcfg_parse_int(&line_pos, " ");
			if (!line_pos)
				continue;

			/* polarization */
			tmp.polarization = tolower(dvbcfg_parse_char(&line_pos, " "));
			if (!line_pos)
				continue;
			if ((tmp.polarization != 'h') &&
			    (tmp.polarization != 'v') &&
			    (tmp.polarization != 'l') &&
			    (tmp.polarization != 'r'))
				continue;

			/* symbol rate */
			tmp.fe_params.u.dvbs.symbol_rate = dvbcfg_parse_int(&line_pos, " ");
			if (!line_pos)
				continue;

			/* fec */
			tmp.fe_params.u.dvbc.fec_inner =
				dvbcfg_parse_setting(&line_pos, " ", dvbcfg_fec_list);
			if (!line_pos)
				continue;

			break;

		case DVBFE_TYPE_DVBT:

			/* parse frequency */
			tmp.fe_params.frequency = dvbcfg_parse_int(&line_pos, " ");
			if (!line_pos)
				continue;

			/* bandwidth */
			tmp.fe_params.u.dvbt.bandwidth =
				dvbcfg_parse_setting(&line_pos, " ", dvbcfg_bandwidth_list);
			if (!line_pos)
				continue;

			/* fec hp */
			tmp.fe_params.u.dvbt.code_rate_HP =
				dvbcfg_parse_setting(&line_pos, " ", dvbcfg_fec_list);
			if (!line_pos)
				continue;

			/* fec lp */
			tmp.fe_params.u.dvbt.code_rate_LP =
				dvbcfg_parse_setting(&line_pos, " ", dvbcfg_fec_list);
			if (!line_pos)
				continue;

			/* constellation */
			tmp.fe_params.u.dvbt.constellation =
				dvbcfg_parse_setting(&line_pos, " ", dvbcfg_constellation_list);
			if (!line_pos)
				continue;

			/* transmission mode */
			tmp.fe_params.u.dvbt.transmission_mode =
				dvbcfg_parse_setting(&line_pos, " ", dvbcfg_transmission_mode_list);
			if (!line_pos)
				continue;

			/* guard interval */
			tmp.fe_params.u.dvbt.guard_interval =
				dvbcfg_parse_setting(&line_pos, " ", dvbcfg_guard_interval_list);
			if (!line_pos)
				continue;

			/* hierarchy */
			tmp.fe_params.u.dvbt.hierarchy_information =
				dvbcfg_parse_setting(&line_pos, " ", dvbcfg_hierarchy_list);
			if (!line_pos)
				continue;

			break;
		}

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
