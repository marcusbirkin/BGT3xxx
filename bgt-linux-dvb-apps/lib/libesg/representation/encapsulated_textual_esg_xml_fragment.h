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

#ifndef _ESG_REPRESENTATION_ENCAPSULATED_TEXTUAL_ESG_XML_FRAGMENT_H
#define _ESG_REPRESENTATION_ENCAPSULATED_TEXTUAL_ESG_XML_FRAGMENT_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * esg_encapsulated_textual_esg_xml_fragment structure.
 */
struct esg_encapsulated_textual_esg_xml_fragment {
	uint16_t esg_xml_fragment_type;
	uint32_t data_length;
	uint8_t *data;
};

/**
 * Process an esg_encapsulated_textual_esg_xml_fragment.
 *
 * @param buffer Binary buffer to decode.
 * @param size Binary buffer size.
 * @return Pointer to an esg_encapsulated_textual_esg_xml_fragment structure, or NULL on error.
 */
extern struct esg_encapsulated_textual_esg_xml_fragment *esg_encapsulated_textual_esg_xml_fragment_decode(uint8_t *buffer, uint32_t size);

/**
 * Free an esg_encapsulated_textual_esg_xml_fragment.
 *
 * @param data_repository Pointer to an esg_encapsulated_textual_esg_xml_fragment structure.
 */
extern void esg_encapsulated_textual_esg_xml_fragment_free(struct esg_encapsulated_textual_esg_xml_fragment *esg_xml_fragment);

#ifdef __cplusplus
}
#endif

#endif
