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

#ifndef _ESG_REPRESENTATION_TEXTUAL_DECODER_INIT_H
#define _ESG_REPRESENTATION_TEXTUAL_DECODER_INIT_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * esg_namespace_prefix structure.
 */
struct esg_namespace_prefix {
	uint16_t prefix_string_ptr;
	uint16_t namespace_uri_ptr;

	struct esg_namespace_prefix *_next;
};

/**
 * esg_fragment_type structure.
 */
struct esg_xml_fragment_type {
	uint16_t xpath_ptr;
	uint16_t xml_fragment_type;

	struct esg_xml_fragment_type *_next;
};

/**
 * esg_textual_decoder_init structure.
 */
struct esg_textual_decoder_init {
	uint8_t version;
	uint8_t num_namespace_prefixes;
	struct esg_namespace_prefix *namespace_prefix_list;
	uint8_t num_fragment_types;
	struct esg_xml_fragment_type *xml_fragment_type_list;
};

/**
 * Process an esg_textual_decoder_init.
 *
 * @param buffer Binary buffer to decode.
 * @param size Binary buffer size.
 * @return Pointer to an esg_textual_decoder_init structure, or NULL on error.
 */
extern struct esg_textual_decoder_init *esg_textual_decoder_init_decode(uint8_t *buffer, uint32_t size);

/**
 * Free an esg_textual_decoder_init.
 *
 * @param decoder_init Pointer to an esg_textual_decoder_init structure.
 */
extern void esg_textual_decoder_init_free(struct esg_textual_decoder_init *decoder_init);

/**
 * Convenience iterator for namespace_prefix_list field of an esg_textual_decoder_init.
 *
 * @param decoder_init The esg_textual_decoder_init pointer.
 * @param namespace_prefix Variable holding a pointer to the current esg_namespace_prefix.
 */
#define esg_textual_decoder_namespace_prefix_list_for_each(decoder_init, namespace_prefix) \
	for ((namespace_prefix) = (decoder_init)->namespace_prefix_list; \
	     (namespace_prefix); \
	     (namespace_prefix) = (namespace_prefix)->_next)

/**
 * Convenience iterator for xml_fragment_type_list field of an esg_textual_decoder_init.
 *
 * @param decoder_init The esg_textual_decoder_init pointer.
 * @param xml_fragment_type Variable holding a pointer to the current esg_xml_fragment_type.
 */
#define esg_textual_decoder_xml_fragment_type_list_for_each(decoder_init, xml_fragment_type) \
	for ((xml_fragment_type) = (decoder_init)->xml_fragment_type_list; \
	     (xml_fragment_type); \
	     (xml_fragment_type) = (xml_fragment_type)->_next)

#ifdef __cplusplus
}
#endif

#endif
