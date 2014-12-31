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
#include <libesg/representation/encapsulated_textual_esg_xml_fragment.h>

struct esg_encapsulated_textual_esg_xml_fragment *esg_encapsulated_textual_esg_xml_fragment_decode(uint8_t *buffer, uint32_t size) {
	struct esg_encapsulated_textual_esg_xml_fragment *esg_xml_fragment;
	uint32_t pos;
	uint32_t length;
	uint8_t offset_pos;

	if ((buffer == NULL) || (size <= 0)) {
		return NULL;
	}

	pos = 0;

	esg_xml_fragment = (struct esg_encapsulated_textual_esg_xml_fragment *) malloc(sizeof(struct esg_encapsulated_textual_esg_xml_fragment));
	memset(esg_xml_fragment, 0, sizeof(struct esg_encapsulated_textual_esg_xml_fragment));

	offset_pos = vluimsbf8(buffer+pos+2, size-pos-2, &length);

	if (size-pos-2 < offset_pos+length) {
		esg_encapsulated_textual_esg_xml_fragment_free(esg_xml_fragment);
		return NULL;
	}

	esg_xml_fragment->esg_xml_fragment_type = (buffer[pos] << 8) | buffer[pos+1];
	pos += 2+offset_pos;

	esg_xml_fragment->data_length = length;
	esg_xml_fragment->data = (uint8_t *) malloc(length);
	memcpy(esg_xml_fragment->data, buffer+pos, length);
	pos += length;

	return esg_xml_fragment;
}

void esg_encapsulated_textual_esg_xml_fragment_free(struct esg_encapsulated_textual_esg_xml_fragment *esg_xml_fragment) {
	if (esg_xml_fragment == NULL) {
		return;
	}

	if (esg_xml_fragment->data) {
		free(esg_xml_fragment->data);
	}

	free(esg_xml_fragment);
}
