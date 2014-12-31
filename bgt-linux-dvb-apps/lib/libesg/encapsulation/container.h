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

#ifndef _ESG_ENCAPSULATION_CONTAINER_H
#define _ESG_ENCAPSULATION_CONTAINER_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * esg_container_structure structure.
 */
struct esg_container_structure {
	uint8_t type;
	uint8_t id;
	uint32_t ptr;
	uint32_t length;

	void *data;

	struct esg_container_structure *_next;
};

/**
 * esg_container_header structure.
 */
struct esg_container_header {
	uint8_t num_structures;
	struct esg_container_structure *structure_list;
};

/**
 * esg_container structure
 */
struct esg_container {
	struct esg_container_header *header;
	uint32_t structure_body_ptr;
	uint32_t structure_body_length;
	uint8_t *structure_body;
};

/**
 * Process an esg_container.
 *
 * @param buffer Binary buffer to decode.
 * @param size Binary buffer size.
 * @return Pointer to an esg_container structure, or NULL on error.
 */
extern struct esg_container *esg_container_decode(uint8_t *buffer, uint32_t size);

/**
 * Free an esg_container.
 *
 * @param container Pointer to an esg_container structure.
 */
extern void esg_container_free(struct esg_container *container);

/**
 * Convenience iterator for structure_list field of an esg_container_header.
 *
 * @param container The esg_container_header pointer.
 * @param structure Variable holding a pointer to the current esg_container_structure.
 */
#define esg_container_header_structure_list_for_each(header, structure) \
	for ((structure) = (header)->structure_list; \
	     (structure); \
	     (structure) = (structure)->_next)

#ifdef __cplusplus
}
#endif

#endif
