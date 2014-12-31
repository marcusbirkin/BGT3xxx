/*
	dvbscan utility

	Copyright (C) 2006 Andrew de Quincey (adq_dvb@lidskialf.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libdvbsec/dvbsec_cfg.h>
#include <libdvbcfg/dvbcfg_scanfile.h>
#include <libdvbapi/dvbdemux.h>
#include "dvbscan.h"


#define OUTPUT_TYPE_RAW 		1
#define OUTPUT_TYPE_CHANNELS 		2
#define OUTPUT_TYPE_VDR12 		3
#define OUTPUT_TYPE_VDR13 		4

#define SERVICE_FILTER_TV		1
#define SERVICE_FILTER_RADIO		2
#define SERVICE_FILTER_OTHER		4
#define SERVICE_FILTER_ENCRYPTED	8

#define TIMEOUT_WAIT_LOCK		2


// transponders we have yet to scan
static struct transponder *toscan = NULL;
static struct transponder *toscan_end = NULL;

// transponders we have scanned
static struct transponder *scanned = NULL;
static struct transponder *scanned_end = NULL;


static void usage(void)
{
	static const char *_usage = "\n"
		" dvbscan: A digital tv channel scanning utility\n"
		" Copyright (C) 2006 Andrew de Quincey (adq_dvb@lidskialf.net)\n\n"
		" usage: dvbscan <options> as follows:\n"
		" -h			help\n"
		" -adapter <id>		adapter to use (default 0)\n"
		" -frontend <id>	frontend to use (default 0)\n"
		" -demux <id>		demux to use (default 0)\n"
		" -secfile <filename>	Optional sec.conf file.\n"
		" -secid <secid>	ID of the SEC configuration to use, one of:\n"
		"			 * UNIVERSAL (default) - Europe, 10800 to 11800 MHz and 11600 to 12700 Mhz,\n"
		" 						 Dual LO, loband 9750, hiband 10600 MHz.\n"
		"			 * DBS - Expressvu, North America, 12200 to 12700 MHz, Single LO, 11250 MHz.\n"
		"			 * STANDARD - 10945 to 11450 Mhz, Single LO, 10000 Mhz.\n"
		"			 * ENHANCED - Astra, 10700 to 11700 MHz, Single LO, 9750 MHz.\n"
		"			 * C-BAND - Big Dish, 3700 to 4200 MHz, Single LO, 5150 Mhz.\n"
		"			 * C-MULTI - Big Dish - Multipoint LNBf, 3700 to 4200 MHz,\n"
		"						Dual LO, H:5150MHz, V:5750MHz.\n"
		"			 * One of the sec definitions from the secfile if supplied\n"
		" -satpos <position>	Specify DISEQC switch position for DVB-S.\n"
		" -inversion <on|off|auto> Specify inversion (default: auto).\n"
		" -uk-ordering 		Use UK DVB-T channel ordering if present.\n"
		" -timeout <secs>	Specify filter timeout to use (standard specced values will be used by default)\n"
		" -filter <filter>	Specify service filter, a comma seperated list of the following tokens:\n"
		" 			 (If no filter is supplied, all services will be output)\n"
		"			 * tv - Output TV channels\n"
		"			 * radio - Output radio channels\n"
		"			 * other - Output other channels\n"
		"			 * encrypted - Output encrypted channels\n"
		" -out raw <filename>|-	 Output in raw format to <filename> or stdout\n"
		"      channels <filename>|-  Output in channels.conf format to <filename> or stdout.\n"
		"      vdr12 <filename>|- Output in vdr 1.2.x format to <filename> or stdout.\n"
		"      vdr13 <filename>|- Output in vdr 1.3.x format to <filename> or stdout.\n"
		" <initial scan file>\n";
	fprintf(stderr, "%s\n", _usage);

	exit(1);
}


static int scan_load_callback(struct dvbcfg_scanfile *channel, void *private_data)
{
	struct dvbfe_info *feinfo = (struct dvbfe_info *) private_data;

	if (channel->fe_type != feinfo->type)
		return 0;

	struct transponder *t = new_transponder();
	append_transponder(t, &toscan, &toscan_end);
	memcpy(&t->params, &channel->fe_params, sizeof(struct dvbfe_parameters));

	add_frequency(t, t->params.frequency);
	t->params.frequency = 0;

	return 0;
}

int main(int argc, char *argv[])
{
	uint32_t i;
	int argpos = 1;
	int adapter_id = 0;
	int frontend_id = 0;
	int demux_id = 0;
	char *secfile = NULL;
	char *secid = NULL;
	int satpos = 0;
	enum dvbfe_spectral_inversion inversion = DVBFE_INVERSION_AUTO;
	int service_filter = -1;
	int uk_ordering = 0;
	int timeout = 5;
	int output_type = OUTPUT_TYPE_RAW;
	char *output_filename = NULL;
	char *scan_filename = NULL;
	struct dvbsec_config sec;
	int valid_sec = 0;

	while(argpos != argc) {
		if (!strcmp(argv[argpos], "-h")) {
			usage();
		} else if (!strcmp(argv[argpos], "-adapter")) {
			if ((argc - argpos) < 2)
				usage();
			if (sscanf(argv[argpos+1], "%i", &adapter_id) != 1)
				usage();
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-frontend")) {
			if ((argc - argpos) < 2)
				usage();
			if (sscanf(argv[argpos+1], "%i", &frontend_id) != 1)
				usage();
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-demux")) {
			if ((argc - argpos) < 2)
				usage();
			if (sscanf(argv[argpos+1], "%i", &demux_id) != 1)
				usage();
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-secfile")) {
			if ((argc - argpos) < 2)
				usage();
			secfile = argv[argpos+1];
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-secid")) {
			if ((argc - argpos) < 2)
				usage();
			secid = argv[argpos+1];
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-satpos")) {
			if ((argc - argpos) < 2)
				usage();
			if (sscanf(argv[argpos+1], "%i", &satpos) != 1)
				usage();
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-inversion")) {
			if ((argc - argpos) < 2)
				usage();
			if (!strcmp(argv[argpos+1], "off")) {
				inversion = DVBFE_INVERSION_OFF;
			} else if (!strcmp(argv[argpos+1], "on")) {
				inversion = DVBFE_INVERSION_ON;
			} else if (!strcmp(argv[argpos+1], "auto")) {
				inversion = DVBFE_INVERSION_AUTO;
			} else {
				usage();
			}
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-uk-ordering")) {
			if ((argc - argpos) < 1)
				usage();
			uk_ordering = 1;
		} else if (!strcmp(argv[argpos], "-timeout")) {
			if ((argc - argpos) < 2)
				usage();
			if (sscanf(argv[argpos+1], "%i", &timeout) != 1)
				usage();
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-filter")) {
			if ((argc - argpos) < 2)
				usage();
			service_filter = 0;
			if (!strstr(argv[argpos+1], "tv")) {
				service_filter |= SERVICE_FILTER_TV;
			}
 			if (!strstr(argv[argpos+1], "radio")) {
				service_filter |= SERVICE_FILTER_RADIO;
 			}
			if (!strstr(argv[argpos+1], "other")) {
				service_filter |= SERVICE_FILTER_OTHER;
			}
			if (!strstr(argv[argpos+1], "encrypted")) {
				service_filter |= SERVICE_FILTER_ENCRYPTED;
			}
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-out")) {
			if ((argc - argpos) < 3)
				usage();
			if (!strcmp(argv[argpos+1], "raw")) {
				output_type = OUTPUT_TYPE_RAW;
			} else if (!strcmp(argv[argpos+1], "channels")) {
				output_type = OUTPUT_TYPE_CHANNELS;
			} else if (!strcmp(argv[argpos+1], "vdr12")) {
				output_type = OUTPUT_TYPE_VDR12;
			} else if (!strcmp(argv[argpos+1], "vdr13")) {
				output_type = OUTPUT_TYPE_VDR13;
			} else {
				usage();
			}
			output_filename = argv[argpos+2];
			if (!strcmp(output_filename, "-"))
				output_filename = NULL;
		} else {
			if ((argc - argpos) != 1)
				usage();
			scan_filename = argv[argpos];
			argpos++;
		}
	}

	// open the frontend & get its type
	struct dvbfe_handle *fe = dvbfe_open(adapter_id, frontend_id, 0);
	if (fe == NULL) {
		fprintf(stderr, "Failed to open frontend\n");
		exit(1);
	}
	struct dvbfe_info feinfo;
	if (dvbfe_get_info(fe, 0, &feinfo, DVBFE_INFO_QUERYTYPE_IMMEDIATE, 0) != 0) {
		fprintf(stderr, "Failed to query frontend\n");
		exit(1);
	}

	// default SEC with a DVBS card
	if ((secid == NULL) && (feinfo.type == DVBFE_TYPE_DVBS))
		secid = "UNIVERSAL";

	// look up SECID if one was supplied
	if (secid != NULL) {
		if (dvbsec_cfg_find(secfile, secid, &sec)) {
			fprintf(stderr, "Unable to find suitable sec/lnb configuration for channel\n");
			exit(1);
		}
		valid_sec = 1;
	}

	// load the initial scan file
	FILE *scan_file = fopen(scan_filename, "r");
	if (scan_file == NULL) {
		fprintf(stderr, "Could not open scan file %s\n", scan_filename);
		exit(1);
	}
	if (dvbcfg_scanfile_parse(scan_file, scan_load_callback, &feinfo) < 0) {
		fprintf(stderr, "Could not parse scan file %s\n", scan_filename);
		exit(1);
	}
	fclose(scan_file);

	// main scan loop
	while(toscan) {
		// get the first item on the toscan list
		struct transponder *tmp = first_transponder(&toscan, &toscan_end);

		// have we already seen this transponder?
		if (seen_transponder(tmp, scanned)) {
			free_transponder(tmp);
			continue;
		}

		// do we have a valid SEC configuration?
		struct dvbsec_config *psec = NULL;
		if (valid_sec)
			psec = &sec;

		// tune it
		int tuned_ok = 0;
		for(i=0; i < tmp->frequency_count; i++) {
			tmp->params.frequency = tmp->frequencies[i];
			if (dvbsec_set(fe,
					psec,
					tmp->polarization,
					(satpos & 0x01) ? DISEQC_SWITCH_B : DISEQC_SWITCH_A,
					(satpos & 0x02) ? DISEQC_SWITCH_B : DISEQC_SWITCH_A,
					&tmp->params,
					0)) {
				fprintf(stderr, "Failed to set frontend\n");
				exit(1);
			}

			// wait for lock
			time_t starttime = time(NULL);
			while((time(NULL) - starttime) < TIMEOUT_WAIT_LOCK) {
				if (dvbfe_get_info(fe, DVBFE_INFO_LOCKSTATUS, &feinfo,
				    			DVBFE_INFO_QUERYTYPE_IMMEDIATE, 0) !=
					DVBFE_INFO_QUERYTYPE_IMMEDIATE) {
					fprintf(stderr, "Unable to query frontend status\n");
					exit(1);
				}
				if (feinfo.lock) {
					tuned_ok = 1;
					break;
				}
				usleep(100000);
			}
		}
		if (!tuned_ok) {
			free_transponder(tmp);
			continue;
		}

		// scan it
		switch(feinfo.type) {
		case DVBFE_TYPE_DVBS:
		case DVBFE_TYPE_DVBC:
		case DVBFE_TYPE_DVBT:
			dvbscan_scan_dvb(fe);
			break;

		case DVBFE_TYPE_ATSC:
			dvbscan_scan_atsc(fe);
			break;
		}

		// add to scanned list.
		append_transponder(tmp, &scanned, &scanned_end);
	}

	// FIXME: output the data

	return 0;
}

int create_section_filter(int adapter, int demux, uint16_t pid, uint8_t table_id)
{
	int demux_fd = -1;
	uint8_t filter[18];
	uint8_t mask[18];

	// open the demuxer
	if ((demux_fd = dvbdemux_open_demux(adapter, demux, 0)) < 0) {
		return -1;
	}

	// create a section filter
	memset(filter, 0, sizeof(filter));
	memset(mask, 0, sizeof(mask));
	filter[0] = table_id;
	mask[0] = 0xFF;
	if (dvbdemux_set_section_filter(demux_fd, pid, filter, mask, 1, 1)) {
		close(demux_fd);
		return -1;
	}

	// done
	return demux_fd;
}
