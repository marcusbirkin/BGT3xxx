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

#include <libesg/types.h>
#include <libesg/representation/textual_decoder_init.h>

struct esg_textual_decoder_init *esg_textual_decoder_init_decode(uint8_t *buffer, uint32_t size) {
	uint32_t pos;
	struct esg_textual_decoder_init *decoder_init;
	struct esg_namespace_prefix *namespace_prefix;
	struct esg_namespace_prefix *last_namespace_prefix;
	struct esg_xml_fragment_type *xml_fragment_type;
	struct esg_xml_fragment_type *last_xml_fragment_type;
	uint32_t decoder_init_length;
	uint8_t num_index;

	if ((buffer == NULL) || (size <= 1)) {
		return NULL;
	}

	pos = 0;

	decoder_init = (struct esg_textual_decoder_init *) malloc(sizeof(struct esg_textual_decoder_init));
	memset(decoder_init, 0, sizeof(struct esg_textual_decoder_init));
	decoder_init->namespace_prefix_list = NULL;
	decoder_init->xml_fragment_type_list = NULL;

	decoder_init->version = buffer[pos];
	pos += 1;

	pos += vluimsbf8(buffer+pos, size-pos, &decoder_init_length);

	if (size < pos + decoder_init_length) {
		esg_textual_decoder_init_free(decoder_init);
		return NULL;
	}

	decoder_init->num_namespace_prefixes = buffer[pos];
	pos += 1;

	last_namespace_prefix = NULL;
	for (num_index = 0; num_index < decoder_init->num_namespace_prefixes; num_index++) {
		namespace_prefix = (struct esg_namespace_prefix *) malloc(sizeof(struct esg_namespace_prefix));
		memset(namespace_prefix, 0, sizeof(struct esg_namespace_prefix));
		namespace_prefix->_next = NULL;

		if (last_namespace_prefix == NULL) {
			decoder_init->namespace_prefix_list = namespace_prefix;
		} else {
			last_namespace_prefix->_next = namespace_prefix;
		}
		last_namespace_prefix = namespace_prefix;

		namespace_prefix->prefix_string_ptr = (buffer[pos] << 8) | buffer[pos+1];
		pos += 2;

		namespace_prefix->namespace_uri_ptr = (buffer[pos] << 8) | buffer[pos+1];
		pos += 2;
	}

	decoder_init->num_fragment_types = buffer[pos];
	pos += 1;

	last_xml_fragment_type = NULL;
	for (num_index = 0; num_index < decoder_init->num_fragment_types; num_index++) {
		xml_fragment_type = (struct esg_xml_fragment_type *) malloc(sizeof(struct esg_xml_fragment_type));
		memset(xml_fragment_type, 0, sizeof(struct esg_xml_fragment_type));
		xml_fragment_type->_next = NULL;

		if (last_xml_fragment_type == NULL) {
			decoder_init->xml_fragment_type_list = xml_fragment_type;
		} else {
			last_xml_fragment_type->_next = xml_fragment_type;
		}
		last_xml_fragment_type = xml_fragment_type;

		xml_fragment_type->xpath_ptr = (buffer[pos] << 8) | buffer[pos+1];
		pos += 2;

		xml_fragment_type->xml_fragment_type = (buffer[pos] << 8) | buffer[pos+1];
		pos += 2;
	}

	return decoder_init;
}

void esg_textual_decoder_init_free(struct esg_textual_decoder_init *decoder_init) {
	struct esg_namespace_prefix *namespace_prefix;
	struct esg_namespace_prefix *next_namespace_prefix;
	struct esg_xml_fragment_type *xml_fragment_type;
	struct esg_xml_fragment_type *next_xml_fragment_type;

	if (decoder_init == NULL) {
		return;
    }

	for(namespace_prefix = decoder_init->namespace_prefix_list; namespace_prefix; namespace_prefix = next_namespace_prefix) {
		next_namespace_prefix = namespace_prefix->_next;
		free(namespace_prefix);
	}

	for(xml_fragment_type = decoder_init->xml_fragment_type_list; xml_fragment_type; xml_fragment_type = next_xml_fragment_type) {
		next_xml_fragment_type = xml_fragment_type->_next;
		free(xml_fragment_type);
	}

	free(decoder_init);
}
