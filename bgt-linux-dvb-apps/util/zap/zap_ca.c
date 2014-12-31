/*
	ZAP utility CA functions

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
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <pthread.h>
#include <libdvben50221/en50221_stdcam.h>
#include "zap_ca.h"


static int zap_ca_info_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint32_t ca_id_count, uint16_t *ca_ids);
static int zap_ai_callback(void *arg, uint8_t slot_id, uint16_t session_number,
			     uint8_t application_type, uint16_t application_manufacturer,
			     uint16_t manufacturer_code, uint8_t menu_string_length,
			     uint8_t *menu_string);
static void *camthread_func(void* arg);

static struct en50221_transport_layer *tl = NULL;
static struct en50221_session_layer *sl = NULL;
static struct en50221_stdcam *stdcam = NULL;

static int ca_resource_connected = 0;

static int camthread_shutdown = 0;
static pthread_t camthread;
static int seenpmt = 0;
static int moveca = 0;

void zap_ca_start(struct zap_ca_params *params)
{
	// create transport layer
	tl = en50221_tl_create(1, 16);
	if (tl == NULL) {
		fprintf(stderr, "Failed to create transport layer\n");
		return;
	}

	// create session layer
	sl = en50221_sl_create(tl, 16);
	if (sl == NULL) {
		fprintf(stderr, "Failed to create session layer\n");
		en50221_tl_destroy(tl);
		return;
	}

	// create the stdcam instance
	stdcam = en50221_stdcam_create(params->adapter_id, params->caslot_num, tl, sl);
	if (stdcam == NULL) {
		en50221_sl_destroy(sl);
		en50221_tl_destroy(tl);
		return;
	}

	// hook up the AI callbacks
	if (stdcam->ai_resource) {
		en50221_app_ai_register_callback(stdcam->ai_resource, zap_ai_callback, stdcam);
	}

	// hook up the CA callbacks
	if (stdcam->ca_resource) {
		en50221_app_ca_register_info_callback(stdcam->ca_resource, zap_ca_info_callback, stdcam);
	}

	// any other stuff
	moveca = params->moveca;

	// start the cam thread
	pthread_create(&camthread, NULL, camthread_func, NULL);
}

void zap_ca_stop(void)
{
	if (stdcam == NULL)
		return;

	// shutdown the cam thread
	camthread_shutdown = 1;
	pthread_join(camthread, NULL);

	// destroy session layer
	en50221_sl_destroy(sl);

	// destroy transport layer
	en50221_tl_destroy(tl);

	// destroy the stdcam
	if (stdcam->destroy)
		stdcam->destroy(stdcam, 1);
}

int zap_ca_new_pmt(struct mpeg_pmt_section *pmt)
{
	uint8_t capmt[4096];
	int size;

	if (stdcam == NULL)
		return -1;

	if (ca_resource_connected) {
		fprintf(stderr, "Received new PMT - sending to CAM...\n");

		// translate it into a CA PMT
		int listmgmt = CA_LIST_MANAGEMENT_ONLY;
		if (seenpmt) {
			listmgmt = CA_LIST_MANAGEMENT_UPDATE;
		}
		seenpmt = 1;

		if ((size = en50221_ca_format_pmt(pmt, capmt, sizeof(capmt), moveca, listmgmt,
						  CA_PMT_CMD_ID_OK_DESCRAMBLING)) < 0) {
			fprintf(stderr, "Failed to format PMT\n");
			return -1;
		}

		// set it
		if (en50221_app_ca_pmt(stdcam->ca_resource, stdcam->ca_session_number, capmt, size)) {
			fprintf(stderr, "Failed to send PMT\n");
			return -1;
		}

		// we've seen this PMT
		return 1;
	}

	return 0;
}

void zap_ca_new_dvbtime(time_t dvb_time)
{
	if (stdcam == NULL)
		return;

	if (stdcam->dvbtime)
		stdcam->dvbtime(stdcam, dvb_time);
}

static void *camthread_func(void* arg)
{
	(void) arg;

	while(!camthread_shutdown) {
		stdcam->poll(stdcam);
	}

	return 0;
}

static int zap_ai_callback(void *arg, uint8_t slot_id, uint16_t session_number,
			     uint8_t application_type, uint16_t application_manufacturer,
			     uint16_t manufacturer_code, uint8_t menu_string_length,
			     uint8_t *menu_string)
{
	(void) arg;
	(void) slot_id;
	(void) session_number;

	printf("CAM Application type: %02x\n", application_type);
	printf("CAM Application manufacturer: %04x\n", application_manufacturer);
	printf("CAM Manufacturer code: %04x\n", manufacturer_code);
	printf("CAM Menu string: %.*s\n", menu_string_length, menu_string);

	return 0;
}

static int zap_ca_info_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint32_t ca_id_count, uint16_t *ca_ids)
{
	(void) arg;
	(void) slot_id;
	(void) session_number;

	printf("CAM supports the following ca system ids:\n");
	uint32_t i;
	for(i=0; i< ca_id_count; i++) {
		printf("  0x%04x\n", ca_ids[i]);
	}
	ca_resource_connected = 1;
	return 0;
}
