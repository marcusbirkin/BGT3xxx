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

#ifndef _ESG_REPRESENTATION_INIT_MESSAGE_H
#define _ESG_REPRESENTATION_INIT_MESSAGE_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * esg_textual_encoding_parameters structure.
 */
struct esg_textual_encoding_parameters {
	uint8_t character_encoding;
};

/**
 * esg_bim_encoding_parameters structure.
 */
struct esg_bim_encoding_parameters {
	uint8_t buffer_size_flag;
	uint8_t position_code_flag;
	uint8_t character_encoding;
	uint32_t buffer_size; // if buffer_size_flag
};

/**
 * esg_init_message structure.
 */
struct esg_init_message {
	uint8_t encoding_version;
	uint8_t indexing_flag;
	uint8_t decoder_init_ptr;
	uint8_t indexing_version; // if indexing_flag
	void *encoding_parameters;
	void *decoder_init;
};

/**
 * Process an esg_init_message.
 *
 * @param buffer Binary buffer to decode.
 * @param size Binary buffer size.
 * @return Pointer to an esg_string_repository structure, or NULL on error.
 */
extern struct esg_init_message *esg_init_message_decode(uint8_t *buffer, uint32_t size);

/**
 * Free an esg_init_message.
 *
 * @param init_message Pointer to an esg_init_message structure.
 */
extern void esg_init_message_free(struct esg_init_message *init_message);

#ifdef __cplusplus
}
#endif

#endif
