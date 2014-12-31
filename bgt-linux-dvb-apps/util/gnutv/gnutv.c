/*
	gnutv utility

	Copyright (C) 2004, 2005 Manu Abraham <abraham.manu@gmail.com>
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

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/poll.h>
#include <libdvbapi/dvbdemux.h>
#include <libdvbapi/dvbaudio.h>
#include <libdvbsec/dvbsec_cfg.h>
#include <libucsi/mpeg/section.h>
#include "gnutv.h"
#include "gnutv_dvb.h"
#include "gnutv_ca.h"
#include "gnutv_data.h"


static void signal_handler(int _signal);

static int quit_app = 0;

void usage(void)
{
	static const char *_usage = "\n"
		" gnutv: A digital tv utility\n"
		" Copyright (C) 2004, 2005, 2006 Manu Abraham (manu@kromtek.com)\n"
		" Copyright (C) 2006 Andrew de Quincey (adq_dvb@lidskialf.net)\n\n"
		" usage: gnutv <options> as follows:\n"
		" -h			help\n"
		" -adapter <id>		adapter to use (default 0)\n"
		" -frontend <id>	frontend to use (default 0)\n"
		" -demux <id>		demux to use (default 0)\n"
		" -caslotnum <id>	ca slot number to use (default 0)\n"
		" -channels <filename>	channels.conf file.\n"
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
		" -buffer <size>	Custom DVR buffer size\n"
		" -out decoder		Output to hardware decoder (default)\n"
		"      decoderabypass	Output to hardware decoder using audio bypass\n"
		"      dvr		Output stream to dvr device\n"
		"      null		Do not output anything\n"
		"      stdout		Output to stdout\n"
		"      file <filename>	Output stream to file\n"
		"      udp <address> <port>			Output stream to address:port using udp\n"
		"      udpif <address> <port> <interface> 	Output stream to address:port using udp\n"
		"							forcing the specified interface\n"
		"      rtp <address> <port>			Output stream to address:port using udp-rtp\n"
		"      rtpif <address> <port> <interface> 	Output stream to address:port using udp-rtp\n"
		"							forcing the specified interface\n"
		" -timeout <secs>	Number of seconds to output channel for\n"
		"				(0=>exit immediately after successful tuning, default is to output forever)\n"
		" -cammenu		Show the CAM menu\n"
		" -nomoveca		Do not attempt to move CA descriptors from stream to programme level\n"
		" <channel name>\n";
	fprintf(stderr, "%s\n", _usage);

	exit(1);
}

int find_channel(struct dvbcfg_zapchannel *channel, void *private_data)
{
	struct dvbcfg_zapchannel *tmpchannel = private_data;

	if (strcmp(channel->name, tmpchannel->name) == 0) {
		memcpy(tmpchannel, channel, sizeof(struct dvbcfg_zapchannel));
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int adapter_id = 0;
	int frontend_id = 0;
	int demux_id = 0;
	int caslot_num = 0;
	char *chanfile = "/etc/channels.conf";
	char *secfile = NULL;
	char *secid = NULL;
	char *channel_name = NULL;
	int output_type = OUTPUT_TYPE_DECODER;
	char *outfile = NULL;
	char *outhost = NULL;
	char *outport = NULL;
	char *outif = NULL;
	struct addrinfo *outaddrs = NULL;
	int timeout = -1;
	int moveca = 1;
	int cammenu = 0;
	int argpos = 1;
	struct gnutv_dvb_params gnutv_dvb_params;
	struct gnutv_ca_params gnutv_ca_params;
	int ffaudiofd = -1;
	int usertp = 0;
	int buffer_size = 0;

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
		} else if (!strcmp(argv[argpos], "-caslotnum")) {
			if ((argc - argpos) < 2)
				usage();
			if (sscanf(argv[argpos+1], "%i", &caslot_num) != 1)
				usage();
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-channels")) {
			if ((argc - argpos) < 2)
				usage();
			chanfile = argv[argpos+1];
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
		} else if (!strcmp(argv[argpos], "-buffer")) {
			if ((argc - argpos) < 2)
				usage();
			if (sscanf(argv[argpos+1], "%i", &buffer_size) != 1)
				usage();
			if (buffer_size < 0)
				usage();
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-out")) {
			if ((argc - argpos) < 2)
				usage();
			if (!strcmp(argv[argpos+1], "decoder")) {
				output_type = OUTPUT_TYPE_DECODER;
			} else if (!strcmp(argv[argpos+1], "decoderabypass")) {
				output_type = OUTPUT_TYPE_DECODER_ABYPASS;
			} else if (!strcmp(argv[argpos+1], "dvr")) {
				output_type = OUTPUT_TYPE_DVR;
			} else if (!strcmp(argv[argpos+1], "null")) {
				output_type = OUTPUT_TYPE_NULL;
			} else if (!strcmp(argv[argpos+1], "stdout")) {
				output_type = OUTPUT_TYPE_STDOUT;
			} else if (!strcmp(argv[argpos+1], "file")) {
				output_type = OUTPUT_TYPE_FILE;
				if ((argc - argpos) < 3)
					usage();
				outfile = argv[argpos+2];
				argpos++;
			} else if ((!strcmp(argv[argpos+1], "udp")) ||
				   (!strcmp(argv[argpos+1], "rtp"))) {
				output_type = OUTPUT_TYPE_UDP;
				if ((argc - argpos) < 4)
					usage();

				if (!strcmp(argv[argpos+1], "rtp"))
					usertp = 1;
				outhost = argv[argpos+2];
				outport = argv[argpos+3];
				argpos+=2;
			} else if ((!strcmp(argv[argpos+1], "udpif")) ||
				   (!strcmp(argv[argpos+1], "rtpif"))) {
				output_type = OUTPUT_TYPE_UDP;
				if ((argc - argpos) < 5)
					usage();

				if (!strcmp(argv[argpos+1], "rtpif"))
					usertp = 1;
				outhost = argv[argpos+2];
				outport = argv[argpos+3];
				outif = argv[argpos+4];
				argpos+=3;
			} else {
				usage();
			}
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-timeout")) {
			if ((argc - argpos) < 2)
				usage();
			if (sscanf(argv[argpos+1], "%i", &timeout) != 1)
				usage();
			argpos+=2;
		} else if (!strcmp(argv[argpos], "-nomoveca")) {
			moveca = 0;
			argpos++;
		} else if (!strcmp(argv[argpos], "-cammenu")) {
			cammenu = 1;
			argpos++;
		} else {
			if ((argc - argpos) != 1)
				usage();
			channel_name = argv[argpos];
			argpos++;
		}
	}

	// the user didn't select anything!
	if ((channel_name == NULL) && (!cammenu))
		usage();

	// resolve host/port
	if ((outhost != NULL) && (outport != NULL)) {
		int res;
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		if ((res = getaddrinfo(outhost, outport, &hints, &outaddrs)) != 0) {
			fprintf(stderr, "Unable to resolve requested address: %s\n", gai_strerror(res));
			exit(1);
		}
	}

	// setup any signals
	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	// start the CA stuff
	gnutv_ca_params.adapter_id = adapter_id;
	gnutv_ca_params.caslot_num = caslot_num;
	gnutv_ca_params.cammenu = cammenu;
	gnutv_ca_params.moveca = moveca;
	gnutv_ca_start(&gnutv_ca_params);

	// frontend setup if a channel name was supplied
	if ((!cammenu) && (channel_name != NULL)) {
		// find the requested channel
		if (strlen(channel_name) >= sizeof(gnutv_dvb_params.channel.name)) {
			fprintf(stderr, "Channel name is too long %s\n", channel_name);
			exit(1);
		}
		FILE *channel_file = fopen(chanfile, "r");
		if (channel_file == NULL) {
			fprintf(stderr, "Could open channel file %s\n", chanfile);
			exit(1);
		}
		memcpy(gnutv_dvb_params.channel.name, channel_name, strlen(channel_name) + 1);
		if (dvbcfg_zapchannel_parse(channel_file, find_channel, &gnutv_dvb_params.channel) != 1) {
			fprintf(stderr, "Unable to find requested channel %s\n", channel_name);
			exit(1);
		}
		fclose(channel_file);

		// default SEC with a DVBS card
		if ((secid == NULL) && (gnutv_dvb_params.channel.fe_type == DVBFE_TYPE_DVBS))
			secid = "UNIVERSAL";

		// look it up if one were supplied
		gnutv_dvb_params.valid_sec = 0;
		if (secid != NULL) {
			if (dvbsec_cfg_find(secfile, secid,
					&gnutv_dvb_params.sec)) {
				fprintf(stderr, "Unable to find suitable sec/lnb configuration for channel\n");
				exit(1);
			}
			gnutv_dvb_params.valid_sec = 1;
		}

		// open the frontend
		gnutv_dvb_params.fe = dvbfe_open(adapter_id, frontend_id, 0);
		if (gnutv_dvb_params.fe == NULL) {
			fprintf(stderr, "Failed to open frontend\n");
			exit(1);
		}

		// failover decoder to dvr output if decoder not available
		if ((output_type == OUTPUT_TYPE_DECODER) ||
		    (output_type == OUTPUT_TYPE_DECODER_ABYPASS)) {
			ffaudiofd = dvbaudio_open(adapter_id, 0);
			if (ffaudiofd < 0) {
				fprintf(stderr, "Cannot open decoder; defaulting to dvr output\n");
				output_type = OUTPUT_TYPE_DVR;
			}
		}

		// start the DVB stuff
		gnutv_dvb_params.adapter_id = adapter_id;
		gnutv_dvb_params.frontend_id = frontend_id;
		gnutv_dvb_params.demux_id = demux_id;
		gnutv_dvb_params.output_type = output_type;
		gnutv_dvb_start(&gnutv_dvb_params);

		// start the data stuff
		gnutv_data_start(output_type, ffaudiofd, adapter_id, demux_id, buffer_size, outfile, outif, outaddrs, usertp);
	}

	// the UI
	time_t start = 0;
	while(!quit_app) {
	        if (gnutv_dvb_locked() && (start == 0))
			start = time(NULL);

		// the timeout
		if ((timeout != -1) && (start != 0)) {
			if ((time(NULL) - start) >= timeout)
				break;
		}

		if (cammenu)
			gnutv_ca_ui();
		else
			usleep(1);
	}

	// stop data handling
	gnutv_data_stop();

	// shutdown DVB stuff
	if (channel_name != NULL)
		gnutv_dvb_stop();

	// shutdown CA stuff
	gnutv_ca_stop();

	// done
	exit(0);
}

static void signal_handler(int _signal)
{
	(void) _signal;

	if (!quit_app) {
		quit_app = 1;
	}
}
