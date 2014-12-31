/*
 * atsc_epg utility
 *
 * Copyright (C) 2009 Yufei Yuan <yfyuan@gmail.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <libdvbapi/dvbfe.h>
#include <libdvbapi/dvbdemux.h>
#include <libucsi/dvb/section.h>
#include <libucsi/atsc/section.h>
#include <libucsi/atsc/types.h>

#define TIMEOUT				60
#define RRT_TIMEOUT			60
#define MAX_NUM_EVENT_TABLES		128
#define TITLE_BUFFER_LEN		4096
#define MESSAGE_BUFFER_LEN		(16 * 1024)
#define MAX_NUM_CHANNELS		16
#define MAX_NUM_EVENTS_PER_CHANNEL	(4 * 24 * 7)

static int atsc_scan_table(int dmxfd, uint16_t pid, enum atsc_section_tag tag,
	void **table_section);

static const char *program;
static int adapter = 0;
static int period = 12; /* hours */
static int frequency;
static int enable_ett = 0;
static int ctrl_c = 0;
static const char *modulation = NULL;
static char separator[80];
void (*old_handler)(int);

struct atsc_string_buffer {
	int buf_len;
	int buf_pos;
	char *string;
};

struct atsc_event_info {
	uint16_t id;
	struct tm start;
	struct tm end;
	int title_pos;
	int title_len;
	int msg_pos;
	int msg_len;
};

struct atsc_eit_section_info {
	uint8_t section_num;
	uint8_t num_events;
	uint8_t num_etms;
	uint8_t num_received_etms;
	struct atsc_event_info **events;
};

struct atsc_eit_info {
	int num_eit_sections;
	struct atsc_eit_section_info *section;
};

struct atsc_channel_info {
	uint8_t num_eits;
	uint8_t service_type;
	char short_name[8];
	uint16_t major_num;
	uint16_t minor_num;
	uint16_t tsid;
	uint16_t prog_num;
	uint16_t src_id;
	struct atsc_eit_info *eit;
	struct atsc_event_info *last_event;
	int event_info_index;
	struct atsc_event_info e[MAX_NUM_EVENTS_PER_CHANNEL];
	struct atsc_string_buffer title_buf;
	struct atsc_string_buffer msg_buf;
};

struct atsc_virtual_channels_info {
	int num_channels;
	uint16_t eit_pid[MAX_NUM_EVENT_TABLES];
	uint16_t ett_pid[MAX_NUM_EVENT_TABLES];
	struct atsc_channel_info ch[MAX_NUM_CHANNELS];
} guide;

struct mgt_table_name {
	uint16_t range;
	const char *string;
};

struct mgt_table_name mgt_tab_name_array[] = {
	{0x0000, "terrestrial VCT with current_next_indictor=1"},
	{0x0001, "terrestrial VCT with current_next_indictor=0"},
	{0x0002, "cable VCT with current_next_indictor=1"},
	{0x0003, "cable VCT with current_next_indictor=0"},
	{0x0004, "channel ETT"},
	{0x0005, "DCCSCT"},
	{0x00FF, "reserved for future ATSC use"},
	{0x017F, "EIT"},
	{0x01FF, "reserved for future ATSC use"},
	{0x027F, "event ETT"},
	{0x02FF, "reserved for future ATSC use"}, /* FIXME */
	{0x03FF, "RRT with rating region"},
	{0x0FFF, "user private"},
	{0x13FF, "reserved for future ATSC use"},
	{0x14FF, "DCCT with dcc_id"},
	{0xFFFF, "reserved for future ATSC use"}
};

const char *channel_modulation_mode[] = {
	"",
	"analog",
	"SCTE mode 1",
	"SCTE mode 2",
	"ATSC 8VSB",
	"ATSC 16VSB"
};

const char *channel_service_type[] = {
	"",
	"analog TV",
	"ATSC digital TV",
	"ATSC audio",
	"ATSC data-only"
};

void *(*table_callback[16])(struct atsc_section_psip *) =
{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	(void *(*)(struct atsc_section_psip *))atsc_mgt_section_codec,
	(void *(*)(struct atsc_section_psip *))atsc_tvct_section_codec,
	(void *(*)(struct atsc_section_psip *))atsc_cvct_section_codec,
	(void *(*)(struct atsc_section_psip *))atsc_rrt_section_codec,
	(void *(*)(struct atsc_section_psip *))atsc_eit_section_codec,
	(void *(*)(struct atsc_section_psip *))atsc_ett_section_codec,
	(void *(*)(struct atsc_section_psip *))atsc_stt_section_codec,
	NULL, NULL
};

static void int_handler(int sig_num)
{
	if(SIGINT != sig_num) {
		return;
	}
	ctrl_c = 1;
}

/* shamelessly stolen from dvbsnoop, but almost not modified */
static uint32_t get_bits(const uint8_t *buf, int startbit, int bitlen)
{
	const uint8_t *b;
	uint32_t mask,tmp_long;
	int bitHigh,i;

	b = &buf[startbit / 8];
	startbit %= 8;

	bitHigh = 8;
	tmp_long = b[0];
	for (i = 0; i < ((bitlen-1) >> 3); i++) {
		tmp_long <<= 8;
		tmp_long  |= b[i+1];
		bitHigh   += 8;
	}

	startbit = bitHigh - startbit - bitlen;
	tmp_long = tmp_long >> startbit;
	mask     = (1ULL << bitlen) - 1;
	return tmp_long & mask;
}

static void usage(void)
{
	fprintf(stderr, "usage: %s [-a <n>] -f <frequency> [-p <period>]"
		" [-m <modulation>] [-t] [-h]\n", program);
}

static void help(void)
{
	fprintf(stderr,
	"\nhelp:\n"
	"%s [-a <n>] -f <frequency> [-p <period>] [-m <modulation>] [-t] [-h]\n"
	"  -a: adapter index to use, (default 0)\n"
	"  -f: tuning frequency\n"
	"  -p: period in hours, (default 12)\n"
	"  -m: modulation ATSC vsb_8|vsb_16 (default vsb_8)\n"
	"  -t: enable ETT to receive program details, if available\n"
	"  -h: display this message\n", program);
}

static int close_frontend(struct dvbfe_handle *fe)
{
	if(NULL == fe) {
		fprintf(stderr, "%s(): NULL pointer detected\n", __FUNCTION__);
	}

	dvbfe_close(fe);

	return 0;
}

static int open_frontend(struct dvbfe_handle **fe)
{
	struct dvbfe_info fe_info;

	if(NULL == (*fe = dvbfe_open(adapter, 0, 0))) {
		fprintf(stderr, "%s(): error calling dvbfe_open()\n",
			__FUNCTION__);
		return -1;
	}
	dvbfe_get_info(*fe, 0, &fe_info, DVBFE_INFO_QUERYTYPE_IMMEDIATE, 0);
	if(DVBFE_TYPE_ATSC != fe_info.type) {
		fprintf(stderr, "%s(): only ATSC frontend supported currently\n",
			__FUNCTION__);
		return -1;
	}
	fe_info.feparams.frequency = frequency;
	fe_info.feparams.inversion = DVBFE_INVERSION_AUTO;
	fe_info.feparams.u.atsc.modulation = DVBFE_ATSC_MOD_VSB_8;
	fprintf(stdout, "tuning to %d Hz, please wait...\n", frequency);
	if(dvbfe_set(*fe, &fe_info.feparams, TIMEOUT * 1000)) {
		fprintf(stderr, "%s(): cannot lock to %d Hz in %d seconds\n",
			__FUNCTION__, frequency, TIMEOUT);
		return -1;
	}
	fprintf(stdout, "tuner locked.\n");

	return 0;
}

#if ENABLE_RRT
/* this is untested as since this part of the library is broken */
static int parse_rrt(int dmxfd)
{
	const enum atsc_section_tag tag = stag_atsc_rating_region;
	struct atsc_rrt_section *rrt;
	struct atsc_text *region_name;
	struct atsc_text_string *atsc_str;
	int i, j, ret;

	i = 0;
	fprintf(stdout, "waiting for RRT: ");
	fflush(stdout);
	while(i < RRT_TIMEOUT) {
		ret = atsc_scan_table(dmxfd, ATSC_BASE_PID, tag, (void **)&rrt);
		if(0 > ret) {
			fprintf(stderr, "%s(): error calling atsc_scan_table()\n",
				__FUNCTION__);
			return -1;
		}
		if(0 == ret) {
			if(RRT_TIMEOUT > i) {
				fprintf(stdout, ".");
				fflush(stdout);
			} else {
				fprintf(stdout, "\nno RRT in %d seconds\n",
					RRT_TIMEOUT);
				return 0;
			}
			i += TIMEOUT;
		} else {
			fprintf(stdout, "\n");
			fflush(stdout);
			break;
		}
	}

	region_name = atsc_rrt_section_rating_region_name_text(rrt);
	atsc_text_strings_for_each(region_name, atsc_str, i) {
		struct atsc_text_string_segment *seg;

		atsc_text_string_segments_for_each(atsc_str, seg, j) {
			const char *c;
			int k;
			if(seg->mode < 0x3E) {
				fprintf(stderr, "%s(): text mode of 0x%02X "
					"not supported yet\n",
					__FUNCTION__, seg->mode);
				return -1;
			}
			c = (const char *)atsc_text_string_segment_bytes(seg);
			for(k = 0; k < seg->number_bytes; k++) {
				fprintf(stdout, "%c", c[k]);
			}
		}
	}

	return 0;
}
#endif

static int parse_stt(int dmxfd)
{
	const enum atsc_section_tag tag = stag_atsc_system_time;
	const struct atsc_stt_section *stt;
	time_t rx_time;
	time_t sys_time;
	int ret;

	ret = atsc_scan_table(dmxfd, ATSC_BASE_PID, tag, (void **)&stt);
	if(0 > ret) {
		fprintf(stderr, "%s(): error calling atsc_scan_table()\n",
			__FUNCTION__);
		return -1;
	}
	if(0 == ret) {
		fprintf(stdout, "no STT in %d seconds\n", TIMEOUT);
		return 0;
	}

	rx_time = atsctime_to_unixtime(stt->system_time);
	time(&sys_time);
	fprintf(stdout, "system time: %s", ctime(&sys_time));
	fprintf(stdout, "TS STT time: %s", ctime(&rx_time));

	return 0;
}

static int parse_tvct(int dmxfd)
{
	int num_sections;
	uint32_t section_pattern;
	const enum atsc_section_tag tag = stag_atsc_terrestrial_virtual_channel;
	struct atsc_tvct_section *tvct;
	struct atsc_tvct_channel *ch;
	struct atsc_channel_info *curr_info;
	int i, k, ret;

	section_pattern = 0;
	num_sections = -1;

	do {
		ret = atsc_scan_table(dmxfd, ATSC_BASE_PID, tag, (void **)&tvct);
		if(0 > ret) {
			fprintf(stderr, "%s(): error calling atsc_scan_table()\n",
			__FUNCTION__);
			return -1;
		}
		if(0 == ret) {
			fprintf(stdout, "no TVCT in %d seconds\n", TIMEOUT);
			return 0;
		}

		if(-1 == num_sections) {
			num_sections = 1 + tvct->head.ext_head.last_section_number;
			if(32 < num_sections) {
				fprintf(stderr, "%s(): no support yet for "
					"tables having more than 32 sections\n",
					__FUNCTION__);
				return -1;
			}
		} else {
			if(num_sections !=
				1 + tvct->head.ext_head.last_section_number) {
				fprintf(stderr,
					"%s(): last section number does not match\n",
					__FUNCTION__);
				return -1;
			}
		}
		if(section_pattern & (1 << tvct->head.ext_head.section_number)) {
			continue;
		}
		section_pattern |= 1 << tvct->head.ext_head.section_number;

		if(MAX_NUM_CHANNELS < guide.num_channels +
			tvct->num_channels_in_section) {
			fprintf(stderr, "%s(): no support for more than %d "
				"virtual channels in a pyhsical channel\n",
				__FUNCTION__, MAX_NUM_CHANNELS);
			return -1;
		}
		curr_info = &guide.ch[guide.num_channels];
		guide.num_channels += tvct->num_channels_in_section;

	atsc_tvct_section_channels_for_each(tvct, ch, i) {
		/* initialize the curr_info structure */
		/* each EIT covers 3 hours */
		curr_info->num_eits = (period / 3) + !!(period % 3);
		while (curr_info->num_eits &&
			(0xFFFF == guide.eit_pid[curr_info->num_eits - 1])) {
			curr_info->num_eits -= 1;
		}
		if(curr_info->eit) {
			fprintf(stderr, "%s(): non-NULL pointer detected "
				"during initialization", __FUNCTION__);
			return -1;
		}
		if(NULL == (curr_info->eit = calloc(curr_info->num_eits,
			sizeof(struct atsc_eit_info)))) {
			fprintf(stderr, "%s(): error calling calloc()\n",
				__FUNCTION__);
			return -1;
		}
		if(NULL == (curr_info->title_buf.string = calloc(TITLE_BUFFER_LEN,
			sizeof(char)))) {
			fprintf(stderr, "%s(): error calling calloc()\n",
				__FUNCTION__);
			return -1;
		}
		curr_info->title_buf.buf_len = TITLE_BUFFER_LEN;
		curr_info->title_buf.buf_pos = 0;

		if(NULL == (curr_info->msg_buf.string = calloc(MESSAGE_BUFFER_LEN,
			sizeof(char)))) {
			fprintf(stderr, "%s(): error calling calloc()\n",
				__FUNCTION__);
			return -1;
		}
		curr_info->msg_buf.buf_len = MESSAGE_BUFFER_LEN;
		curr_info->msg_buf.buf_pos = 0;

		for(k = 0; k < 7; k++) {
			curr_info->short_name[k] =
				get_bits((const uint8_t *)ch->short_name,
				k * 16, 16);
		}
		curr_info->service_type = ch->service_type;
		curr_info->major_num = ch->major_channel_number;
		curr_info->minor_num = ch->minor_channel_number;
		curr_info->tsid = ch->channel_TSID;
		curr_info->prog_num = ch->program_number;
		curr_info->src_id = ch->source_id;
		curr_info++;
		}
	} while(section_pattern != (uint32_t)((1 << num_sections) - 1));

	return 0;
}

static int match_event(struct atsc_eit_info *eit, uint16_t event_id,
	struct atsc_event_info **event, uint8_t *curr_index)
{
	int j, k;
	struct atsc_eit_section_info *section;

	if(NULL == eit || NULL == event || NULL == curr_index) {
		fprintf(stderr, "%s(): NULL pointer detected\n", __FUNCTION__);
		return -1;
	}

	for(j = 0; j < eit->num_eit_sections; j++) {
		section = &eit->section[j];

		for(k = 0; k < section->num_events; k++) {
			if(section->events[k] && section->events[k]->id ==
				event_id) {
				*event = section->events[k];
				break;
			}
		}
		if(*event) {
			*curr_index = j;
			break;
		}
	}

	return 0;
}

static int parse_message(struct atsc_channel_info *channel,
	struct atsc_ett_section *ett, struct atsc_event_info *event)
{
	int i, j;
	struct atsc_text *text;
	struct atsc_text_string *str;

	if(NULL == ett || NULL == event || NULL == channel) {
		fprintf(stderr, "%s(): NULL pointer detected\n", __FUNCTION__);
		return -1;
	}

	text = atsc_ett_section_extended_text_message(ett);
	atsc_text_strings_for_each(text, str, i) {
		struct atsc_text_string_segment *seg;

		atsc_text_string_segments_for_each(str, seg, j) {
			event->msg_pos = channel->msg_buf.buf_pos;
			if(0 > atsc_text_segment_decode(seg,
				(uint8_t **)&channel->msg_buf.string,
				(size_t *)&channel->msg_buf.buf_len,
				(size_t *)&channel->msg_buf.buf_pos)) {
				fprintf(stderr, "%s(): error calling "
					"atsc_text_segment_decode()\n",
					__FUNCTION__);
				return -1;
			}
			event->msg_len = 1 + channel->msg_buf.buf_pos -
				event->msg_pos;
		}
	}

	return 0;
}

static int parse_ett(int dmxfd, int index, uint16_t pid)
{
	uint8_t curr_index;
	uint32_t section_pattern;
	const enum atsc_section_tag tag = stag_atsc_extended_text;
	struct atsc_eit_info *eit;
	struct atsc_ett_section *ett;
	struct atsc_channel_info *channel;
	struct atsc_event_info *event;
	struct atsc_eit_section_info *section;
	uint16_t source_id, event_id;
	int c, ret;

	if(0xFFFF == guide.ett_pid[index]) {
		return 0;
	}

	for(c = 0; c < guide.num_channels; c++) {
		channel = &guide.ch[c];
		eit = &channel->eit[index];

		section_pattern = 0;
		while(section_pattern !=
			(uint32_t)((1 << eit->num_eit_sections) - 1)) {
			if(ctrl_c) {
				return 0;
			}
			ret = atsc_scan_table(dmxfd, pid, tag, (void **)&ett);
			fprintf(stdout, ".");
			fflush(stdout);
			if(0 > ret) {
				fprintf(stderr, "%s(): error calling "
					"atsc_scan_table()\n", __FUNCTION__);
				return -1;
			}
			if(0 == ret) {
				fprintf(stdout, "no ETT %d in %d seconds\n",
					index, TIMEOUT);
				return 0;
			}

			source_id = ett->ETM_source_id;
			event_id = ett->ETM_sub_id;
			if(source_id != channel->src_id) {
				continue;
			}

			event = NULL;
			if(match_event(eit, event_id, &event, &curr_index)) {
				fprintf(stderr, "%s(): error calling "
					"match_event()\n", __FUNCTION__);
				return -1;
			}
			if(NULL == event) {
				continue;
			}
			if(section_pattern & (1 << curr_index)) {
				/* the section has been filled, so skip,
				 * not consider version yet
				 */
				continue;
			}
			if(event->msg_len) {
				/* the message has been filled */
				continue;
			}

			if(parse_message(channel, ett, event)) {
				fprintf(stderr, "%s(): error calling "
					"parse_message()\n", __FUNCTION__);
				return -1;
			}
			section = &eit->section[curr_index];
			if(++section->num_received_etms == section->num_etms) {
				section_pattern |= 1 << curr_index;
			}
		}
	}

	return 0;
}

static int parse_events(struct atsc_channel_info *curr_info,
	struct atsc_eit_section *eit, struct atsc_eit_section_info *section)
{
	int i, j, k;
	struct atsc_eit_event *e;
	time_t start_time, end_time;

	if(NULL == curr_info || NULL == eit) {
		fprintf(stderr, "%s(): NULL pointer detected\n", __FUNCTION__);
		return -1;
	}

	atsc_eit_section_events_for_each(eit, e, i) {
		struct atsc_text *title;
		struct atsc_text_string *str;
		struct atsc_event_info *e_info =
			&curr_info->e[curr_info->event_info_index];

		if(0 == i && curr_info->last_event) {
			if(e->event_id == curr_info->last_event->id) {
				section->events[i] = NULL;
				/* skip if it's the same event spanning
				 * over sections
				 */
				continue;
			}
		}
		curr_info->event_info_index += 1;
		section->events[i] = e_info;
		e_info->id = e->event_id;
		start_time = atsctime_to_unixtime(e->start_time);
		end_time = start_time + e->length_in_seconds;
		localtime_r(&start_time, &e_info->start);
		localtime_r(&end_time, &e_info->end);
		if(0 != e->ETM_location && 3 != e->ETM_location) {
			/* FIXME assume 1 and 2 is interchangable as of now */
			section->num_etms++;
		}

		title = atsc_eit_event_name_title_text(e);
		if (title == NULL)
			continue;
		atsc_text_strings_for_each(title, str, j) {
			struct atsc_text_string_segment *seg;

			atsc_text_string_segments_for_each(str, seg, k) {
				e_info->title_pos = curr_info->title_buf.buf_pos;
				if(0 > atsc_text_segment_decode(seg,
					(uint8_t **)&curr_info->title_buf.string,
					(size_t *)&curr_info->title_buf.buf_len,
					(size_t *)&curr_info->title_buf.buf_pos)) {
					fprintf(stderr, "%s(): error calling "
						"atsc_text_segment_decode()\n",
						__FUNCTION__);
					return -1;
				}
				e_info->title_len = curr_info->title_buf.buf_pos -
					e_info->title_pos + 1;
			}
		}
	}

	return 0;
}

static int parse_eit(int dmxfd, int index, uint16_t pid)
{
	int num_sections;
	uint8_t section_num;
	uint8_t curr_channel_index;
	uint32_t section_pattern;
	const enum atsc_section_tag tag = stag_atsc_event_information;
	struct atsc_eit_section *eit;
	struct atsc_channel_info *curr_info;
	struct atsc_eit_info *eit_info;
	struct atsc_eit_section_info *section;
	uint16_t source_id;
	uint32_t eit_instance_pattern = 0;
	int i, k, ret;

	while(eit_instance_pattern !=
		(uint32_t)((1 << guide.num_channels) - 1)) {
		source_id = 0xFFFF;
		section_pattern = 0;
		num_sections = -1;

		do {
			ret = atsc_scan_table(dmxfd, pid, tag, (void **)&eit);
			fprintf(stdout, ".");
			fflush(stdout);
			if(0 > ret) {
				fprintf(stderr, "%s(): error calling "
					"atsc_scan_table()\n", __FUNCTION__);
				return -1;
			}
			if(0 == ret) {
				fprintf(stdout, "no EIT %d in %d seconds\n",
					index, TIMEOUT);
				return 0;
			}

			if(0xFFFF == source_id) {
			source_id = atsc_eit_section_source_id(eit);
			for(k = 0; k < guide.num_channels; k++) {
				if(source_id == guide.ch[k].src_id) {
					curr_info = &guide.ch[k];
					curr_channel_index = k;
					if(0 == index) {
						curr_info->last_event = NULL;
					}
					break;
				}
			}
			if(k == guide.num_channels) {
				fprintf(stderr, "%s(): cannot find source_id "
					"0x%04X in the EIT\n",
					__FUNCTION__, source_id);
				return -1;
			}
			} else {
				if(source_id !=
					atsc_eit_section_source_id(eit)) {
					continue;
				}
			}
			if(eit_instance_pattern & (1 << curr_channel_index)) {
				/* we have received this instance,
				 * so quit quick
				 */
				break;
			}

			if(-1 == num_sections) {
				num_sections = 1 +
					eit->head.ext_head.last_section_number;
				if(32 < num_sections) {
					fprintf(stderr,
						"%s(): no support yet for "
						"tables having more than "
						"32 sections\n", __FUNCTION__);
					return -1;
				}
			} else {
				if(num_sections != 1 +
					eit->head.ext_head.last_section_number) {
					fprintf(stderr,
						"%s(): last section number "
						"does not match\n",
						__FUNCTION__);
					return -1;
				}
			}
			if(section_pattern &
				(1 << eit->head.ext_head.section_number)) {
				continue;
			}
			section_pattern |= 1 << eit->head.ext_head.section_number;

			eit_info = &curr_info->eit[index];
			if(NULL == (eit_info->section =
				realloc(eit_info->section,
				(eit_info->num_eit_sections + 1) *
				sizeof(struct atsc_eit_section_info)))) {
				fprintf(stderr,
					"%s(): error calling realloc()\n",
					__FUNCTION__);
				return -1;
			}
			section_num = eit->head.ext_head.section_number;
			if(0 == eit_info->num_eit_sections) {
				eit_info->num_eit_sections = 1;
				section = eit_info->section;
			} else {
				/* have to sort it into section order
				 * (temporal order)
				 */
				for(i = 0; i < eit_info->num_eit_sections; i++) {
					if(eit_info->section[i].section_num >
						section_num) {
						break;
					}
				}
				memmove(&eit_info->section[i + 1],
					&eit_info->section[i],
					(eit_info->num_eit_sections - i) *
					sizeof(struct atsc_eit_section_info));
				section = &eit_info->section[i - 1];
				section = &eit_info->section[i];
				eit_info->num_eit_sections += 1;
			}

			section->section_num = section_num;
			section->num_events = eit->num_events_in_section;
			section->num_etms = 0;
			section->num_received_etms = 0;
			if(NULL == (section->events = calloc(section->num_events,
				sizeof(struct atsc_event_info *)))) {
				fprintf(stderr, "%s(): error calling calloc()\n",
					__FUNCTION__);
				return -1;
			}
			if(parse_events(curr_info, eit, section)) {
				fprintf(stderr, "%s(): error calling "
					"parse_events()\n", __FUNCTION__);
				return -1;
			}
		} while(section_pattern != (uint32_t)((1 << num_sections) - 1));
		eit_instance_pattern |= 1 << curr_channel_index;
	}

	for(i = 0; i < guide.num_channels; i++) {
		struct atsc_channel_info *channel = &guide.ch[i];
		struct atsc_eit_info *ei = &channel->eit[index];
		struct atsc_eit_section_info *s;

		if(0 == ei->num_eit_sections) {
			channel->last_event = NULL;
			continue;
		}
		s = &ei->section[ei->num_eit_sections - 1];
		/* BUG: it's incorrect when last section has no event */
		if(0 == s->num_events) {
			channel->last_event = NULL;
			continue;
		}
		channel->last_event = s->events[s->num_events - 1];
	}

	return 0;
}

static int parse_mgt(int dmxfd)
{
	const enum atsc_section_tag tag = stag_atsc_master_guide;
	struct atsc_mgt_section *mgt;
	struct atsc_mgt_table *t;
	int i, j, ret;

	ret = atsc_scan_table(dmxfd, ATSC_BASE_PID, tag, (void **)&mgt);
	if(0 > ret) {
		fprintf(stderr, "%s(): error calling atsc_scan_table()\n",
			__FUNCTION__);
		return -1;
	}
	if(0 == ret) {
		fprintf(stdout, "no MGT in %d seconds\n", TIMEOUT);
		return 0;
	}

	fprintf(stdout, "MGT table:\n");
	atsc_mgt_section_tables_for_each(mgt, t, i) {
		struct mgt_table_name table;

	for(j = 0; j < (int)(sizeof(mgt_tab_name_array) /
		sizeof(struct mgt_table_name)); j++) {
		if(t->table_type > mgt_tab_name_array[j].range) {
			continue;
		}
		table = mgt_tab_name_array[j];
		if(0 == j || mgt_tab_name_array[j - 1].range + 1 ==
			mgt_tab_name_array[j].range) {
			j = -1;
		} else {
			j = t->table_type - mgt_tab_name_array[j - 1].range - 1;
			if(0x017F == table.range) {
				guide.eit_pid[j] = t->table_type_PID;
			} else if (0x027F == table.range) {
				guide.ett_pid[j] = t->table_type_PID;
			}
		}
		break;
	}

		fprintf(stdout, "  %2d: type = 0x%04X, PID = 0x%04X, %s", i,
		    t->table_type, t->table_type_PID, table.string);
		if(-1 != j) {
		    fprintf(stdout, " %d", j);
		}
		fprintf(stdout, "\n");
	}

	return 0;
}

static int cleanup_guide(void)
{
	int i, j, k;

	for(i = 0; i < guide.num_channels; i++) {
		struct atsc_channel_info *channel = &guide.ch[i];

		if(channel->title_buf.string) {
			free(channel->title_buf.string);
		}
		if(channel->msg_buf.string) {
			free(channel->msg_buf.string);
		}
		for(j = 0; j < channel->num_eits; j++) {
			struct atsc_eit_info *eit = &channel->eit[j];

			for(k = 0; k < eit->num_eit_sections; k++) {
				struct atsc_eit_section_info *section =
					&eit->section[k];
				if(section->num_events) {
					free(section->events);
				}
			}
			if(k) {
				free(eit->section);
			}
		}
		if(j) {
			free(channel->eit);
		}
	}

	return 0;
}

static int print_events(struct atsc_channel_info *channel,
	struct atsc_eit_section_info *section)
{
	int m;
	char line[256];

	if(NULL == section) {
		fprintf(stderr, "%s(): NULL pointer detected", __FUNCTION__);
		return -1;
	}
	for(m = 0; m < section->num_events; m++) {
		struct atsc_event_info *event =
			section->events[m];

		if(NULL == event) {
			continue;
		}
		fprintf(stdout, "|%02d:%02d--%02d:%02d| ",
			event->start.tm_hour, event->start.tm_min,
			event->end.tm_hour, event->end.tm_min);
		snprintf(line, event->title_len, "%s",
			&channel->title_buf.string[event->title_pos]);
		line[event->title_len] = '\0';
		fprintf(stdout, "%s\n", line);
		if(event->msg_len) {
			int len = event->msg_len;
			int pos = event->msg_pos;
			size_t part;

			do {
				part = len > 255 ? 255 : len;
				snprintf(line, part, "%s",
					&channel->msg_buf.string[pos]);
				line[part] = '\0';
				fprintf(stdout, "%s", line);
				len -= part;
				pos += part;
			} while(0 < len);
			fprintf(stdout, "\n");
		}
	}
	return 0;
}

static int print_guide(void)
{
	int i, j, k;

	fprintf(stdout, "%s\n", separator);
	for(i = 0; i < guide.num_channels; i++) {
		struct atsc_channel_info *channel = &guide.ch[i];

		fprintf(stdout, "%d.%d  %s\n", channel->major_num,
			channel->minor_num, channel->short_name);
		for(j = 0; j < channel->num_eits; j++) {
			struct atsc_eit_info *eit = &channel->eit[j];

			for(k = 0; k < eit->num_eit_sections; k++) {
				struct atsc_eit_section_info *section =
					&eit->section[k];
				if(print_events(channel, section)) {
					fprintf(stderr, "%s(): error calling "
						"print_events()\n", __FUNCTION__);
					return -1;
				}
			}
		}
		fprintf(stdout, "%s\n", separator);
	}

	return 0;
}

static int open_demux(int *dmxfd)
{
	if((*dmxfd = dvbdemux_open_demux(adapter, 0, 0)) < 0) {
		fprintf(stderr, "%s(): error calling dvbdemux_open_demux()\n",
			__FUNCTION__);
		return -1;
	}
	return 0;
}

static int close_demux(int dmxfd)
{
	if(dvbdemux_stop(dmxfd)) {
		fprintf(stderr, "%s(): error calling dvbdemux_stop()\n",
			__FUNCTION__);
		return -1;
	}
	return 0;
}

/* used other utilities as template and generalized here */
static int atsc_scan_table(int dmxfd, uint16_t pid, enum atsc_section_tag tag,
	void **table_section)
{
	uint8_t filter[18];
	uint8_t mask[18];
	unsigned char sibuf[4096];
	int size;
	int ret;
	struct pollfd pollfd;
	struct section *section;
	struct section_ext *section_ext;
	struct atsc_section_psip *psip;

	/* create a section filter for the table */
	memset(filter, 0, sizeof(filter));
	memset(mask, 0, sizeof(mask));
	filter[0] = tag;
	mask[0] = 0xFF;
	if(dvbdemux_set_section_filter(dmxfd, pid, filter, mask, 1, 1)) {
		fprintf(stderr, "%s(): error calling atsc_scan_table()\n",
			__FUNCTION__);
		return -1;
	}

	/* poll for data */
	pollfd.fd = dmxfd;
	pollfd.events = POLLIN | POLLERR |POLLPRI;
	if((ret = poll(&pollfd, 1, TIMEOUT * 1000)) < 0) {
		if(ctrl_c) {
			return 0;
		}
		fprintf(stderr, "%s(): error calling poll()\n", __FUNCTION__);
		return -1;
	}

	if(0 == ret) {
		return 0;
	}

	/* read it */
	if((size = read(dmxfd, sibuf, sizeof(sibuf))) < 0) {
		fprintf(stderr, "%s(): error calling read()\n", __FUNCTION__);
		return -1;
	}

	/* parse section */
	section = section_codec(sibuf, size);
	if(NULL == section) {
		fprintf(stderr, "%s(): error calling section_codec()\n",
			__FUNCTION__);
		return -1;
	}

	section_ext = section_ext_decode(section, 0);
	if(NULL == section_ext) {
		fprintf(stderr, "%s(): error calling section_ext_decode()\n",
			__FUNCTION__);
		return -1;
	}

	psip = atsc_section_psip_decode(section_ext);
	if(NULL == psip) {
		fprintf(stderr,
			"%s(): error calling atsc_section_psip_decode()\n",
			__FUNCTION__);
		return -1;
	}

	*table_section = table_callback[tag & 0x0F](psip);
	if(NULL == *table_section) {
		fprintf(stderr, "%s(): error decode table section\n",
			__FUNCTION__);
		return -1;
	}

	return 1;
}

int main(int argc, char *argv[])
{
	int i, dmxfd;
	struct dvbfe_handle *fe;

	program = argv[0];

	if(1 == argc) {
		usage();
		exit(-1);
	}

	for( ; ; ) {
		char c;

		if(-1 == (c = getopt(argc, argv, "a:f:p:m:th"))) {
			break;
		}

		switch(c) {
		case 'a':
			adapter = strtoll(optarg, NULL, 0);
			break;

		case 'f':
			frequency = strtol(optarg, NULL, 0);
			break;

		case 'p':
			period = strtol(optarg, NULL, 0);
			/* each table covers 3 hours */
			if((3 * MAX_NUM_EVENT_TABLES) < period) {
				period = 3 * MAX_NUM_EVENT_TABLES;
			}
			break;

		case 'm':
			/* just stub, so far ATSC only has VSB_8 */
			modulation = optarg;
			break;

		case 't':
			enable_ett = 1;
			break;

		case 'h':
			help();
			exit(0);

		default:
			usage();
			exit(-1);
		}
	}

	memset(separator, '-', sizeof(separator));
	separator[79] = '\0';
	memset(&guide, 0, sizeof(struct atsc_virtual_channels_info));
	memset(guide.eit_pid, 0xFF, MAX_NUM_EVENT_TABLES * sizeof(uint16_t));
	memset(guide.ett_pid, 0xFF, MAX_NUM_EVENT_TABLES * sizeof(uint16_t));

	if(open_frontend(&fe)) {
		fprintf(stderr, "%s(): error calling open_frontend()\n",
			__FUNCTION__);
		return -1;
	}

	if(open_demux(&dmxfd)) {
		fprintf(stderr, "%s(): error calling open_demux()\n",
			__FUNCTION__);
		return -1;
	}

	if(parse_stt(dmxfd)) {
		fprintf(stderr, "%s(): error calling parse_stt()\n",
			__FUNCTION__);
		return -1;
	}

	if(parse_mgt(dmxfd)) {
		fprintf(stderr, "%s(): error calling parse_mgt()\n",
			__FUNCTION__);
		return -1;
	}

	if(parse_tvct(dmxfd)) {
		fprintf(stderr, "%s(): error calling parse_tvct()\n",
			__FUNCTION__);
		return -1;
	}

#ifdef ENABLE_RRT
	if(parse_rrt(dmxfd)) {
		fprintf(stderr, "%s(): error calling parse_rrt()\n",
			__FUNCTION__);
		return -1;
	}
#endif

	fprintf(stdout, "receiving EIT ");
	for(i = 0; i < guide.ch[0].num_eits; i++) {
		if(parse_eit(dmxfd, i, guide.eit_pid[i])) {
			fprintf(stderr, "%s(): error calling parse_eit()\n",
				__FUNCTION__);
			return -1;
		}
	}
	fprintf(stdout, "\n");

	old_handler = signal(SIGINT, int_handler);
	if(enable_ett) {
		fprintf(stdout, "receiving ETT ");
		for(i = 0; i < guide.ch[0].num_eits; i++) {
			if(0xFFFF != guide.ett_pid[i]) {
				if(parse_ett(dmxfd, i, guide.ett_pid[i])) {
					fprintf(stderr, "%s(): error calling "
						"parse_eit()\n", __FUNCTION__);
					return -1;
				}
			}
			if(ctrl_c) {
				break;
			}
		}
		fprintf(stdout, "\n");
	}
	signal(SIGINT, old_handler);

	if(print_guide()) {
		fprintf(stderr, "%s(): error calling print_guide()\n",
			__FUNCTION__);
		return -1;
	}

	if(cleanup_guide()) {
		fprintf(stderr, "%s(): error calling cleanup_guide()\n",
			__FUNCTION__);
		return -1;
	}

	if(close_demux(dmxfd)) {
		fprintf(stderr, "%s(): error calling close_demux()\n",
			__FUNCTION__);
		return -1;
	}

	if(close_frontend(fe)) {
		fprintf(stderr, "%s(): error calling close_demux()\n",
			__FUNCTION__);
		return -1;
	}

	return 0;
}
