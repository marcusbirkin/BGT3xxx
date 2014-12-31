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

#ifndef DVBSCAN_H
#define DVBSCAN_H 1

#include <libdvbapi/dvbfe.h>
#include <libdvbsec/dvbsec_api.h>
#include <libucsi/types.h>

/**
 * A stream which is part of a service.
 */
struct stream
{
	uint8_t stream_type;
	iso639lang_t language;

	struct stream *next;
};

/**
 * A service (programme) which is part of a transponder.
 */
struct service
{
	/**
	 * Service identification stuff. Strings are in UTF-8.
	 */
	uint16_t service_id;
	char *provider_name;
	char *service_name;

	/**
	 * Pids common to the whole service.
	 */
	uint16_t pmt_pid;
	uint16_t pcr_pid;

	/**
	 * CA stuff.
	 */
	uint16_t *ca_ids;
	uint32_t ca_ids_count;
	uint8_t is_scrambled;

	/**
	 * BBC channel number (-1 if unknown).
	 */
	int bbc_channel_number;

	/**
	 * Streams composing this service.
	 */
	struct stream *streams;
	struct stream *streams_end;

	/**
	 * Next service in list.
	 */
	struct service *next;
};

/**
 * A collection of multiplexed services.
 */
struct transponder
{
	/**
	 * we need to have a seperate list of frequencies since the
	 * DVB standard allows a frequency list descriptor of alternate
	 * frequencies to be supplied.
	 */
	uint32_t *frequencies;
	uint32_t frequency_count;

	/**
	 * The rest of the tuning parameters.
	 */
	struct dvbfe_parameters params;

	/**
	 * DVBS specific parameters
	 */
	enum dvbsec_diseqc_polarization polarization;
	int oribital_position;

	/**
	 * Numerical IDs
	 */
	uint16_t network_id;
	uint16_t original_network_id;
	uint16_t transport_stream_id;

	/**
	 * Services detected on this transponder.
	 */
	struct service *services;
	struct service *services_end;

	/**
	 * Next item in list.
	 */
	struct transponder *next;
};

extern void append_transponder(struct transponder *t, struct transponder **tlist, struct transponder **tlist_end);
extern struct transponder *new_transponder(void);
extern void free_transponder(struct transponder *t);
extern int seen_transponder(struct transponder *t, struct transponder *checklist);
extern void add_frequency(struct transponder *t, uint32_t frequency);
extern struct transponder *first_transponder(struct transponder **tlist, struct transponder **tlist_end);

extern int create_section_filter(int adapter, int demux, uint16_t pid, uint8_t table_id);
extern void dvbscan_scan_dvb(struct dvbfe_handle *fe);
extern void dvbscan_scan_atsc(struct dvbfe_handle *fe);

#endif
