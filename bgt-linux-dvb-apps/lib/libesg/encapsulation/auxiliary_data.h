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

#ifndef _ESG_ENCAPSULATION_AUXILIARY_DATA_H
#define _ESG_ENCAPSULATION_AUXILIARY_DATA_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * esg_any_attribute structure.
 */
struct esg_any_attribute {
	uint8_t version_id;
	uint8_t *extension;

	struct esg_any_attribure *_next;
};

/**
 * esg_binary_header structure.
 */
struct esg_binary_header {
	uint16_t encoding_metadatauri_mimetype;
	struct esg_any_attribute *any_attribute_list;
};

/**
 * esg_encapsulated_aux_data struct.
 */
struct esg_encapsulated_aux_data {
	struct esg_binary_header *binary_header;
	uint32_t aux_data_length;
	uint8_t aux_data;
};

#ifdef __cplusplus
}
#endif

#endif
