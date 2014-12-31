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

#include <libesg/encapsulation/fragment_management_information.h>

struct esg_encapsulation_structure *esg_encapsulation_structure_decode(uint8_t *buffer, uint32_t size) {
	uint32_t pos;
	struct esg_encapsulation_structure *structure;
	struct esg_encapsulation_entry *entry;
	struct esg_encapsulation_entry *last_entry;

	if ((buffer == NULL) || (size <= 2)) {
		return NULL;
	}

	pos = 0;

	structure = (struct esg_encapsulation_structure *) malloc(sizeof(struct esg_encapsulation_structure));
	memset(structure, 0, sizeof(struct esg_encapsulation_structure));
	structure->entry_list = NULL;

	// Encapsulation header
	structure->header = (struct esg_encapsulation_header *) malloc(sizeof(struct esg_encapsulation_header));
	// buffer[pos] reserved
	structure->header->fragment_reference_format = buffer[pos+1];
	pos += 2;

	// Encapsulation entry list
	last_entry = NULL;
	while (size > pos) {
		entry = (struct esg_encapsulation_entry *) malloc(sizeof(struct esg_encapsulation_entry));
		memset(entry, 0, sizeof(struct esg_encapsulation_entry));
		entry->_next = NULL;

		if (last_entry == NULL) {
			structure->entry_list = entry;
		} else {
			last_entry->_next = entry;
		}
		last_entry = entry;

		// Fragment reference
		switch (structure->header->fragment_reference_format) {
			case 0x21: {
				entry->fragment_reference = (struct esg_fragment_reference *) malloc(sizeof(struct esg_fragment_reference));
				memset(entry->fragment_reference, 0, sizeof(struct esg_fragment_reference));

				entry->fragment_reference->fragment_type = buffer[pos];
				pos += 1;

				entry->fragment_reference->data_repository_offset = (buffer[pos] << 16) | (buffer[pos+1] << 8) | buffer[pos+2];
				pos += 3;

				break;
			}
			default: {
				esg_encapsulation_structure_free(structure);
				return NULL;
			}
		}

		// Fragment version & id
		entry->fragment_version = buffer[pos];
		pos += 1;

		entry->fragment_id = (buffer[pos] << 16) | (buffer[pos+1] << 8) | buffer[pos+2];
		pos += 3;
	}

	return structure;
}

void esg_encapsulation_structure_free(struct esg_encapsulation_structure *structure) {
	struct esg_encapsulation_entry *entry;
	struct esg_encapsulation_entry *next_entry;

	if (structure == NULL) {
		return;
    }

	if (structure->header) {
		free(structure->header);
	}

	if (structure->entry_list) {
		for(entry = structure->entry_list; entry; entry = next_entry) {
			next_entry = entry->_next;
			if (entry->fragment_reference) {
				free(entry->fragment_reference);
			}
			free(entry);
		}

		free(structure->entry_list);
	}

	free(structure);
}
