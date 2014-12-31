/*
	ZAP utility

	Copyright (C) 2004, 2005 Manu Abraham <abraham.manu@gmail.com>
	Copyright (C) 2006 Andrew de Quincey (adq_dvb@lidskialf.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation; either version 2.1 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
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
#include "zap_dvb.h"
#include "zap_ca.h"


static void signal_handler(int _signal);

static int quit_app = 0;

void usage(void)
{
	static const char *_usage = "\n"
		" ZAP: A zapping application\n"
		" Copyright (C) 2004, 2005, 2006 Manu Abraham (manu@kromtek.com)\n"
		" Copyright (C) 2006 Andrew de Quincey (adq_dvb@lidskialf.net)\n\n"
		" usage: zap <options> as follows:\n"
		" -h			help\n"
		" -adapter <id>		adapter to use (default 0)\n"
		" -frontend <id>	frontend to use (default 0)\n"
		" -demux <id>		demux to use (default 0)\n"
		" -caslotnum <id>	ca slot number to use (default 0)\n"
		" -channels <filename>	channels.conf file.\n"
		" -secfile <filename>	Optional sec.conf file.\n"
		" -secid <secid>	ID of the SEC configuration to use, one of:\n"
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
	int moveca = 1;
	int argpos = 1;
	struct zap_dvb_params zap_dvb_params;
	struct zap_ca_params zap_ca_params;

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
		} else if (!strcmp(argv[argpos], "-nomoveca")) {
			moveca = 0;
			argpos++;
		} else {
			if ((argc - argpos) != 1)
				usage();
			channel_name = argv[argpos];
			argpos++;
		}
	}

	// the user didn't select anything!
	if (channel_name == NULL)
		usage();

	// setup any signals
	signal(SIGINT, signal_handler);
	signal(SIGPIPE, SIG_IGN);

	// start the CA stuff
	zap_ca_params.adapter_id = adapter_id;
	zap_ca_params.caslot_num = caslot_num;
	zap_ca_params.moveca = moveca;
	zap_ca_start(&zap_ca_params);

	// find the requested channel
	if (strlen(channel_name) >= sizeof(zap_dvb_params.channel.name)) {
		fprintf(stderr, "Channel name is too long %s\n", channel_name);
		exit(1);
	}
	FILE *channel_file = fopen(chanfile, "r");
	if (channel_file == NULL) {
		fprintf(stderr, "Could open channel file %s\n", chanfile);
		exit(1);
	}
	memcpy(zap_dvb_params.channel.name, channel_name, strlen(channel_name) + 1);
	if (dvbcfg_zapchannel_parse(channel_file, find_channel, &zap_dvb_params.channel) != 1) {
		fprintf(stderr, "Unable to find requested channel %s\n", channel_name);
		exit(1);
	}
	fclose(channel_file);

	// default SEC with a DVBS card
	if ((secid == NULL) && (zap_dvb_params.channel.fe_type == DVBFE_TYPE_DVBS))
		secid = "UNIVERSAL";

	// look it up if one were supplied
	zap_dvb_params.valid_sec = 0;
	if (secid != NULL) {
		if (dvbsec_cfg_find(secfile, secid,
				&zap_dvb_params.sec)) {
			fprintf(stderr, "Unable to find suitable sec/lnb configuration for channel\n");
			exit(1);
		}
		zap_dvb_params.valid_sec = 1;
	}

	// open the frontend
	zap_dvb_params.fe = dvbfe_open(adapter_id, frontend_id, 0);
	if (zap_dvb_params.fe == NULL) {
		fprintf(stderr, "Failed to open frontend\n");
		exit(1);
	}

	// start the DVB stuff
	zap_dvb_params.adapter_id = adapter_id;
	zap_dvb_params.frontend_id = frontend_id;
	zap_dvb_params.demux_id = demux_id;
	zap_dvb_start(&zap_dvb_params);

	// the UI
	while(!quit_app) {
		sleep(1);
	}

	// shutdown DVB stuff
	if (channel_name != NULL)
		zap_dvb_stop();

	// shutdown CA stuff
	zap_ca_stop();

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
