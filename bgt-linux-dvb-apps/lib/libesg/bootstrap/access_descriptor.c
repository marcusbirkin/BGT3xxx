/*
 * ESG parser
 *
 * Copyright (C) 2006 Stephane Este-Gracias (sestegra@free.fr)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <stdlib.h>
#include <string.h>

#include <libesg/bootstrap/access_descriptor.h>

struct esg_access_descriptor *esg_access_descriptor_decode(uint8_t *buffer, uint32_t size) {
	uint32_t pos;
	struct esg_access_descriptor *access_descriptor;
	struct esg_entry *entry;
	struct esg_entry *last_entry;
	uint32_t entry_length;
	uint16_t entry_index;
	uint8_t ip_index;

	if ((buffer == NULL) || (size <= 2)) {
		return NULL;
	}

	pos = 0;

	access_descriptor = (struct esg_access_descriptor *) malloc(sizeof(struct esg_access_descriptor));
	memset(access_descriptor, 0, sizeof(struct esg_access_descriptor));
	access_descriptor->entry_list = NULL;

	access_descriptor->n_o_entries = (buffer[pos] << 8) | buffer[pos+1];
	pos += 2;

    last_entry = NULL;
	for (entry_index = 0; entry_index < access_descriptor->n_o_entries; entry_index++) {
		entry = (struct esg_entry *) malloc(sizeof(struct esg_entry));
		memset(entry, 0, sizeof(struct esg_entry));
		entry->_next = NULL;

		if (last_entry == NULL) {
			access_descriptor->entry_list = entry;
		} else {
			last_entry->_next = entry;
		}
		last_entry = entry;

		entry->version = buffer[pos];
		pos += 1;

		pos += vluimsbf8(buffer + pos, size - pos, &entry_length);

		if (size < pos + entry_length) {
			esg_access_descriptor_free(access_descriptor);
			return NULL;
		}

		entry->multiple_stream_transport = (buffer[pos] & 0x80) ? 1 : 0;
		entry->ip_version_6 = (buffer[pos] & 0x40) ? 1 : 0;
		pos += 1;

		entry->provider_id = (buffer[pos] << 8) | buffer[pos+1];
		pos += 2;

		if (entry->ip_version_6) {
			for (ip_index = 0; ip_index < 16; ip_index++) {
				entry->source_ip.ipv6[ip_index] = buffer[pos+ip_index];
				entry->destination_ip.ipv6[ip_index] = buffer[pos+16+ip_index];
			}
			pos += 32;
		} else {
			for (ip_index = 0; ip_index < 4; ip_index++) {
				entry->source_ip.ipv4[ip_index] = buffer[pos+ip_index];
				entry->destination_ip.ipv4[ip_index] = buffer[pos+4+ip_index];
			}
			pos += 8;
		}
		entry->port = (buffer[pos] << 8) | buffer[pos+1];
		pos += 2;

		entry->tsi = (buffer[pos] << 8) | buffer[pos+1];
		pos += 2;
	}

	return access_descriptor;
}

void esg_access_descriptor_free(struct esg_access_descriptor *access_descriptor) {
	struct esg_entry *entry;
	struct esg_entry *next_entry;

	if (access_descriptor == NULL) {
		return;
    }

	for(entry = access_descriptor->entry_list; entry; entry = next_entry) {
		next_entry = entry->_next;
		free(entry);
	}

	free(access_descriptor);
}
