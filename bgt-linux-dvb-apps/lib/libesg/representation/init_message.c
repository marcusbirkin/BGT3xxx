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

#include <libesg/representation/init_message.h>
#include <libesg/representation/textual_decoder_init.h>
#include <libesg/representation/bim_decoder_init.h>

struct esg_init_message *esg_init_message_decode(uint8_t *buffer, uint32_t size) {
	uint32_t pos;
	struct esg_init_message *init_message;

	if ((buffer == NULL) || (size <= 3)) {
		return NULL;
	}

	pos = 0;

	init_message = (struct esg_init_message *) malloc(sizeof(struct esg_init_message));
	memset(init_message, 0, sizeof(struct esg_init_message));

	init_message->encoding_version = buffer[pos];
	pos += 1;

	init_message->indexing_flag = (buffer[pos] & 0x80) >> 7;
	pos += 1;

	init_message->decoder_init_ptr = buffer[pos];
	pos += 1;

	if (init_message->indexing_flag) {
		init_message->indexing_version = buffer[pos];
		pos += 1;
	}

	switch (init_message->encoding_version) {
		case 0xF1: {
			struct esg_bim_encoding_parameters *encoding_parameters = (struct esg_bim_encoding_parameters *) malloc(sizeof(struct esg_bim_encoding_parameters));
			memset(encoding_parameters, 0, sizeof(struct esg_bim_encoding_parameters));
			init_message->encoding_parameters = (void *) encoding_parameters;

			encoding_parameters->buffer_size_flag = (buffer[pos] & 0x80) >> 7;
			encoding_parameters->position_code_flag = (buffer[pos] & 0x40) >> 6;
			pos += 1;

			encoding_parameters->character_encoding = buffer[pos];
			pos += 1;

			if (encoding_parameters->buffer_size_flag) {
				encoding_parameters->buffer_size = (buffer[pos] << 16) | (buffer[pos+1] << 8) | buffer[pos+2];
				pos += 3;
			}

// TODO
//			init_message->decoder_init = (void *) esg_bim_decoder_init_decode(buffer + init_message->decoder_init_ptr, size - init_message->decoder_init_ptr);
			break;
		}
		case 0xF2:
		case 0xF3: {
			struct esg_textual_encoding_parameters *encoding_parameters = (struct esg_textual_encoding_parameters *) malloc(sizeof(struct esg_textual_encoding_parameters));
			memset(encoding_parameters, 0, sizeof(struct esg_textual_encoding_parameters));
			init_message->encoding_parameters = (void *) encoding_parameters;

			encoding_parameters->character_encoding = buffer[pos];
			pos += 1;

			init_message->decoder_init = (void *) esg_textual_decoder_init_decode(buffer + init_message->decoder_init_ptr, size - init_message->decoder_init_ptr);
			break;
		}
		default: {
			esg_init_message_free(init_message);
			return NULL;
		}
	}

	return init_message;
}

void esg_init_message_free(struct esg_init_message *init_message) {
	if (init_message == NULL) {
		return;
	}

	if (init_message->encoding_parameters) {
		free(init_message->encoding_parameters);
	}

	if (init_message->decoder_init) {
		free(init_message->decoder_init);
	}

	free(init_message);
}
