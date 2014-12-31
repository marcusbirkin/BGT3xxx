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

#ifndef _ESG_ENCAPSULATION_FRAGMENT_MANAGEMENT_INFORMATION_H
#define _ESG_ENCAPSULATION_FRAGMENT_MANAGEMENT_INFORMATION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * esg_encapsulation_header structure.
 */
struct esg_encapsulation_header {
	uint8_t fragment_reference_format;
};

/**
 * esg_fragment_reference structure.
 */
struct esg_fragment_reference {
	uint8_t fragment_type;
	uint32_t data_repository_offset;
};

/**
 * esg_encapsulation_entry structure.
 */
struct esg_encapsulation_entry {
	struct esg_fragment_reference *fragment_reference;
	uint8_t fragment_version;
	uint32_t fragment_id;

	struct esg_encapsulation_entry *_next;
};

/**
 * esg_encapsulation_structure structure.
 */
struct esg_encapsulation_structure {
	struct esg_encapsulation_header *header;
	struct esg_encapsulation_entry *entry_list;
};

/**
 * Process an esg_encapsulation_structure.
 *
 * @param buffer Binary buffer to decode.
 * @param size Binary buffer size.
 * @return Pointer to an esg_encapsulation_structure structure, or NULL on error.
 */
extern struct esg_encapsulation_structure *esg_encapsulation_structure_decode(uint8_t *buffer, uint32_t size);

/**
 * Free an esg_encapsulation_structure.
 *
 * @param container Pointer to an esg_container structure.
 */
extern void esg_encapsulation_structure_free(struct esg_encapsulation_structure *structure);

/**
 * Convenience iterator for entry_list field of an esg_encapsulation_structure.
 *
 * @param structure The esg_encapsulation_structure pointer.
 * @param entry Variable holding a pointer to the current esg_encapsulation_entry.
 */
#define esg_encapsulation_structure_entry_list_for_each(structure, entry) \
	for ((entry) = (structure)->entry_list; \
	     (entry); \
	     (entry) = (entry)->_next)

#ifdef __cplusplus
}
#endif

#endif
