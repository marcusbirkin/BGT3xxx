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

#ifndef _ESG_BOOTSTRAP_ACCESS_DESCRIPTOR_H
#define _ESG_BOOTSTRAP_ACCESS_DESCRIPTOR_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libesg/types.h>

/**
 * esg_entry structure.
 */
struct esg_entry {
	uint8_t version;
	uint8_t multiple_stream_transport;
	uint8_t ip_version_6;
	uint16_t provider_id;
	union esg_ip_address source_ip;
	union esg_ip_address destination_ip;
	uint16_t port;
	uint16_t tsi;

	struct esg_entry *_next;
};

/**
 * esg_access_descriptor structure.
 */
struct esg_access_descriptor {
	uint16_t n_o_entries;
	struct esg_entry *entry_list;
};

/**
 * Process an esg_access_descriptor.
 *
 * @param buffer Binary buffer to decode.
 * @param size Binary buffer size.
 * @return Pointer to an esg_access_descriptor structure, or NULL on error.
 */
extern struct esg_access_descriptor *esg_access_descriptor_decode(uint8_t *buffer, uint32_t size);

/**
 * Free an esg_access_descriptor.
 *
 * @param esg Pointer to an esg_access_descriptor structure.
 */
extern void esg_access_descriptor_free(struct esg_access_descriptor *access_descriptor);

/**
 * Convenience iterator for esg_entry_list field of an esg_access_descriptor.
 *
 * @param access_descriptor The esg_access_descriptor pointer.
 * @param entry Variable holding a pointer to the current esg_entry.
 */
#define esg_access_descriptor_entry_list_for_each(access_descriptor, entry) \
	for ((entry) = (access_descriptor)->entry_list; \
	     (entry); \
	     (entry) = (entry)->_next)

#ifdef __cplusplus
}
#endif

#endif
