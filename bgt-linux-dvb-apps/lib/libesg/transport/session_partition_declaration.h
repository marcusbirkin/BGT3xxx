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

#ifndef _ESG_TRANSPORT_SESSION_PARTITION_DECLARATION_H
#define _ESG_TRANSPORT_SESSION_PARTITION_DECLARATION_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <libesg/types.h>

/**
 * esg_session_field structure.
 */
struct esg_session_field {
	uint16_t identifier;
	uint16_t encoding;
	uint8_t length;

	struct esg_session_field *_next;
};

/**
 * esg_session_ip_stream_field_value union.
 */
union esg_session_ip_stream_field_value {
	uint8_t *string;
	uint16_t unsigned_short;
};

/**
 * esg_session_ip_stream_field structure.
 */
struct esg_session_ip_stream_field {
	union esg_session_ip_stream_field_value *start_field_value;
	union esg_session_ip_stream_field_value *end_field_value;

	struct esg_session_ip_stream_field *_next;
};

/**
 * esg_session_ip_stream structure.
 */
struct esg_session_ip_stream {
	uint8_t id;
	union esg_ip_address source_ip;
	union esg_ip_address destination_ip;
	uint16_t port;
	uint16_t session_id;
	struct esg_session_ip_stream_field *field_list;

	struct esg_session_ip_stream *_next;
};

/**
 * esg_session_partition_declaration structure.
 */
struct esg_session_partition_declaration {
	uint8_t num_fields;
	uint8_t overlapping;
	struct esg_session_field *field_list;
	uint8_t n_o_ip_streams;
	uint8_t ip_version_6;
	struct esg_session_ip_stream *ip_stream_list;
};

/**
 * Process an esg_session_partition_declaration.
 *
 * @param buffer Binary buffer to decode.
 * @param size Binary buffer size.
 * @return Pointer to an esg_session_partition_declaration structure, or NULL on error.
 */
extern struct esg_session_partition_declaration *esg_session_partition_declaration_decode(uint8_t *buffer, uint32_t size);

/**
 * Free an esg_session_partition_declaration.
 *
 * @param esg Pointer to an esg_session_partition_declaration structure.
 */
extern void esg_session_partition_declaration_free(struct esg_session_partition_declaration *partition);

/**
 * Convenience iterator for field_list field of an esg_session_partition_declaration.
 *
 * @param partition The esg_session_partition_declaration pointer.
 * @param field Variable holding a pointer to the current esg_session_field.
 */
#define esg_session_partition_declaration_field_list_for_each(partition, field) \
	for ((field) = (partition)->field_list; \
	     (field); \
	     (field) = (field)->_next)

/**
 * Convenience iterator for ip_stream_list field of an esg_session_partition_declaration.
 *
 * @param partition The esg_session_partition_declaration pointer.
 * @param ip_stream Variable holding a pointer to the current esg_session_ip_stream.
 */
#define esg_session_partition_declaration_ip_stream_list_for_each(partition, ip_stream) \
	for ((ip_stream) = (partition)->ip_stream_list; \
	     (ip_stream); \
	     (ip_stream) = (ip_stream)->_next)

/**
 * Convenience iterator for field_list field of an esg_session_ip_stream.
 *
 * @param ip_stream The esg_session_ip_stream pointer.
 * @param field Variable holding a pointer to the current esg_session_ip_stream.
 */
#define esg_session_ip_stream_field_list_for_each(ip_stream, field) \
	for ((field) = (ip_stream)->field_list; \
	     (field); \
	     (field) = (field)->_next)

#ifdef __cplusplus
}
#endif

#endif
