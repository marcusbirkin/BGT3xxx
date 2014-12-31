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
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <pthread.h>
#include <libdvben50221/en50221_stdcam.h>
#include "gnutv.h"
#include "gnutv_ca.h"



#define MMI_STATE_CLOSED 0
#define MMI_STATE_OPEN 1
#define MMI_STATE_ENQ 2
#define MMI_STATE_MENU 3

static int gnutv_ca_info_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint32_t ca_id_count, uint16_t *ca_ids);
static int gnutv_ai_callback(void *arg, uint8_t slot_id, uint16_t session_number,
			     uint8_t application_type, uint16_t application_manufacturer,
			     uint16_t manufacturer_code, uint8_t menu_string_length,
			     uint8_t *menu_string);

static int gnutv_mmi_close_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				    uint8_t cmd_id, uint8_t delay);
static int gnutv_mmi_display_control_callback(void *arg, uint8_t slot_id, uint16_t session_number,
					      uint8_t cmd_id, uint8_t mmi_mode);
static int gnutv_mmi_enq_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				  uint8_t blind_answer, uint8_t expected_answer_length,
				  uint8_t *text, uint32_t text_size);
static int gnutv_mmi_menu_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				   struct en50221_app_mmi_text *title,
				   struct en50221_app_mmi_text *sub_title,
				   struct en50221_app_mmi_text *bottom,
				   uint32_t item_count, struct en50221_app_mmi_text *items,
				   uint32_t item_raw_length, uint8_t *items_raw);
static void *camthread_func(void* arg);

static struct en50221_transport_layer *tl = NULL;
static struct en50221_session_layer *sl = NULL;
static struct en50221_stdcam *stdcam = NULL;

static int ca_resource_connected = 0;
static int mmi_state = MMI_STATE_CLOSED;
static int mmi_enq_blind;
static int mmi_enq_length;

static int camthread_shutdown = 0;
static pthread_t camthread;
int moveca = 0;
int seenpmt = 0;
int cammenu = 0;

char ui_line[256];
uint32_t ui_linepos = 0;


void gnutv_ca_start(struct gnutv_ca_params *params)
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
		en50221_app_ai_register_callback(stdcam->ai_resource, gnutv_ai_callback, stdcam);
	}

	// hook up the CA callbacks
	if (stdcam->ca_resource) {
		en50221_app_ca_register_info_callback(stdcam->ca_resource, gnutv_ca_info_callback, stdcam);
	}

	// hook up the MMI callbacks
	if (params->cammenu) {
		if (stdcam->mmi_resource) {
			en50221_app_mmi_register_close_callback(stdcam->mmi_resource, gnutv_mmi_close_callback, stdcam);
			en50221_app_mmi_register_display_control_callback(stdcam->mmi_resource, gnutv_mmi_display_control_callback, stdcam);
			en50221_app_mmi_register_enq_callback(stdcam->mmi_resource, gnutv_mmi_enq_callback, stdcam);
			en50221_app_mmi_register_menu_callback(stdcam->mmi_resource, gnutv_mmi_menu_callback, stdcam);
			en50221_app_mmi_register_list_callback(stdcam->mmi_resource, gnutv_mmi_menu_callback, stdcam);
		} else {
			fprintf(stderr, "CAM Menus are not supported by this interface hardware\n");
			exit(1);
		}
	}

	// any other stuff
	moveca = params->moveca;
	cammenu = params->cammenu;

	// start the cam thread
	pthread_create(&camthread, NULL, camthread_func, NULL);
}

void gnutv_ca_stop(void)
{
	if (stdcam == NULL)
		return;

	// shutdown the cam thread
	camthread_shutdown = 1;
	pthread_join(camthread, NULL);

	// destroy the stdcam
	if (stdcam->destroy)
		stdcam->destroy(stdcam, 1);

	// destroy session layer
	en50221_sl_destroy(sl);

	// destroy transport layer
	en50221_tl_destroy(tl);
}

void gnutv_ca_ui(void)
{
	// make up polling structure for stdin
	struct pollfd pollfd;
	pollfd.fd = 0;
	pollfd.events = POLLIN|POLLPRI|POLLERR;

	if (stdcam == NULL)
		return;

	// is there a character?
	if (poll(&pollfd, 1, 10) != 1)
		return;
	if (pollfd.revents & POLLERR)
		return;

	// try to read the character
	char c;
	if (read(0, &c, 1) != 1)
		return;
	if (c == '\r') {
		return;
	} else if (c == '\n') {
		switch(mmi_state) {
		case MMI_STATE_CLOSED:
		case MMI_STATE_OPEN:
			if ((ui_linepos == 0) && (ca_resource_connected)) {
				en50221_app_ai_entermenu(stdcam->ai_resource, stdcam->ai_session_number);
			}
			break;

		case MMI_STATE_ENQ:
			if (ui_linepos == 0) {
				en50221_app_mmi_answ(stdcam->mmi_resource, stdcam->mmi_session_number,
							MMI_ANSW_ID_CANCEL, NULL, 0);
			} else {
				en50221_app_mmi_answ(stdcam->mmi_resource, stdcam->mmi_session_number,
							MMI_ANSW_ID_ANSWER, (uint8_t*) ui_line, ui_linepos);
			}
			mmi_state = MMI_STATE_OPEN;
			break;

		case MMI_STATE_MENU:
			ui_line[ui_linepos] = 0;
			en50221_app_mmi_menu_answ(stdcam->mmi_resource, stdcam->mmi_session_number,
							atoi(ui_line));
			mmi_state = MMI_STATE_OPEN;
			break;
		}
		ui_linepos = 0;
	} else {
		if (ui_linepos < (sizeof(ui_line)-1)) {
			ui_line[ui_linepos++] = c;
		}
	}
}

int gnutv_ca_new_pmt(struct mpeg_pmt_section *pmt)
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

void gnutv_ca_new_dvbtime(time_t dvb_time)
{
	if (stdcam == NULL)
		return;

	if (stdcam->dvbtime)
		stdcam->dvbtime(stdcam, dvb_time);
}

static void *camthread_func(void* arg)
{
	(void) arg;
	int entered_menu = 0;

	while(!camthread_shutdown) {
		stdcam->poll(stdcam);

		if ((!entered_menu) && cammenu && ca_resource_connected && stdcam->mmi_resource) {
			en50221_app_ai_entermenu(stdcam->ai_resource, stdcam->ai_session_number);
			entered_menu = 1;
		}
	}

	return 0;
}

static int gnutv_ai_callback(void *arg, uint8_t slot_id, uint16_t session_number,
			     uint8_t application_type, uint16_t application_manufacturer,
			     uint16_t manufacturer_code, uint8_t menu_string_length,
			     uint8_t *menu_string)
{
	(void) arg;
	(void) slot_id;
	(void) session_number;

	fprintf(stderr, "CAM Application type: %02x\n", application_type);
	fprintf(stderr, "CAM Application manufacturer: %04x\n", application_manufacturer);
	fprintf(stderr, "CAM Manufacturer code: %04x\n", manufacturer_code);
	fprintf(stderr, "CAM Menu string: %.*s\n", menu_string_length, menu_string);

	return 0;
}

static int gnutv_ca_info_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint32_t ca_id_count, uint16_t *ca_ids)
{
	(void) arg;
	(void) slot_id;
	(void) session_number;

	fprintf(stderr, "CAM supports the following ca system ids:\n");
	uint32_t i;
	for(i=0; i< ca_id_count; i++) {
		fprintf(stderr, "  0x%04x\n", ca_ids[i]);
	}
	ca_resource_connected = 1;
	return 0;
}

static int gnutv_mmi_close_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				    uint8_t cmd_id, uint8_t delay)
{
	(void) arg;
	(void) slot_id;
	(void) session_number;
	(void) cmd_id;
	(void) delay;

	// note: not entirely correct as its supposed to delay if asked
	mmi_state = MMI_STATE_CLOSED;
	return 0;
}

static int gnutv_mmi_display_control_callback(void *arg, uint8_t slot_id, uint16_t session_number,
					      uint8_t cmd_id, uint8_t mmi_mode)
{
	struct en50221_app_mmi_display_reply_details reply;
	(void) arg;
	(void) slot_id;

	// don't support any commands but set mode
	if (cmd_id != MMI_DISPLAY_CONTROL_CMD_ID_SET_MMI_MODE) {
		en50221_app_mmi_display_reply(stdcam->mmi_resource, session_number,
					      MMI_DISPLAY_REPLY_ID_UNKNOWN_CMD_ID, &reply);
		return 0;
	}

	// we only support high level mode
	if (mmi_mode != MMI_MODE_HIGH_LEVEL) {
		en50221_app_mmi_display_reply(stdcam->mmi_resource, session_number,
					      MMI_DISPLAY_REPLY_ID_UNKNOWN_MMI_MODE, &reply);
		return 0;
	}

	// ack the high level open
	reply.u.mode_ack.mmi_mode = mmi_mode;
	en50221_app_mmi_display_reply(stdcam->mmi_resource, session_number,
				      MMI_DISPLAY_REPLY_ID_MMI_MODE_ACK, &reply);
	mmi_state = MMI_STATE_OPEN;
	return 0;
}

static int gnutv_mmi_enq_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				  uint8_t blind_answer, uint8_t expected_answer_length,
				  uint8_t *text, uint32_t text_size)
{
	(void) arg;
	(void) slot_id;
	(void) session_number;

	fprintf(stderr, "%.*s: ", text_size, text);
	fflush(stdout);

	mmi_enq_blind = blind_answer;
	mmi_enq_length = expected_answer_length;
	mmi_state = MMI_STATE_ENQ;
	return 0;
}

static int gnutv_mmi_menu_callback(void *arg, uint8_t slot_id, uint16_t session_number,
				   struct en50221_app_mmi_text *title,
				   struct en50221_app_mmi_text *sub_title,
				   struct en50221_app_mmi_text *bottom,
				   uint32_t item_count, struct en50221_app_mmi_text *items,
				   uint32_t item_raw_length, uint8_t *items_raw)
{
	(void) arg;
	(void) slot_id;
	(void) session_number;
	(void) item_raw_length;
	(void) items_raw;

	fprintf(stderr, "------------------------------\n");

	if (title->text_length) {
		fprintf(stderr, "%.*s\n", title->text_length, title->text);
	}
	if (sub_title->text_length) {
		fprintf(stderr, "%.*s\n", sub_title->text_length, sub_title->text);
	}

	uint32_t i;
	fprintf(stderr, "0. Quit menu\n");
	for(i=0; i< item_count; i++) {
		fprintf(stderr, "%i. %.*s\n", i+1, items[i].text_length, items[i].text);
	}

	if (bottom->text_length) {
		fprintf(stderr, "%.*s\n", bottom->text_length, bottom->text);
	}
	fprintf(stderr, "Enter option: ");
	fflush(stdout);

	mmi_state = MMI_STATE_MENU;
	return 0;
}
