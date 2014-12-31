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

#include <libesg/encapsulation/container.h>
#include <libesg/encapsulation/fragment_management_information.h>
#include <libesg/encapsulation/data_repository.h>
#include <libesg/encapsulation/string_repository.h>
#include <libesg/representation/init_message.h>
#include <libesg/transport/session_partition_declaration.h>

struct esg_container *esg_container_decode(uint8_t *buffer, uint32_t size) {
	uint32_t pos;
	struct esg_container *container;
	struct esg_container_structure *structure;
	struct esg_container_structure *last_structure;
	uint8_t structure_index;

	if ((buffer == NULL) || (size <= 1)) {
		return NULL;
	}

	pos = 0;

	container = (struct esg_container *) malloc(sizeof(struct esg_container));
	memset(container, 0, sizeof(struct esg_container));

	// Container header
	container->header = (struct esg_container_header *) malloc(sizeof(struct esg_container_header));
	memset(container->header, 0, sizeof(struct esg_container_header));

	container->header->num_structures = buffer[pos];
	pos += 1;

	if (size < pos + (container->header->num_structures * 8)) {
		esg_container_free(container);
		return NULL;
	}

	last_structure = NULL;
	for (structure_index = 0; structure_index < container->header->num_structures; structure_index++) {
		structure = (struct esg_container_structure *) malloc(sizeof(struct esg_container_structure));
		memset(structure, 0, sizeof(struct esg_container_structure));
		structure->_next = NULL;

		if (last_structure == NULL) {
			container->header->structure_list = structure;
		} else {
			last_structure->_next = structure;
		}
		last_structure = structure;

		structure->type = buffer[pos];
		pos += 1;

		structure->id = buffer[pos];
		pos += 1;

		structure->ptr = (buffer[pos] << 16) | (buffer[pos+1] << 8) | buffer[pos+2];
		pos += 3;

		structure->length = (buffer[pos] << 16) | (buffer[pos+1] << 8) | buffer[pos+2];
		pos += 3;

		if (size < (structure->ptr + structure->length)) {
			esg_container_free(container);
			return NULL;
		}

		// Decode structure
		switch (structure->type) {
			case 0x01: {
				switch (structure->id) {
					case 0x00: {
						structure->data = (void *) esg_encapsulation_structure_decode(buffer + structure->ptr, structure->length);
						break;
					}
					default: {
						esg_container_free(container);
						return NULL;
					}
				}
				break;
			}
			case 0x02: {
				switch (structure->id) {
					case 0x00: {
						structure->data = (void *) esg_string_repository_decode(buffer + structure->ptr, structure->length);
						break;
					}
					default: {
						esg_container_free(container);
						return NULL;
					}
				}
				break;
			}
			case 0x03: {
				//TODO
				break;
			}
			case 0x04: {
				//TODO
				break;
			}
			case 0x05: {
				//TODO
				break;
			}
			case 0xE0: {
				switch (structure->id) {
					case 0x00: {
						structure->data = (void *) esg_data_repository_decode(buffer + structure->ptr, structure->length);
						break;
					}
					default: {
						esg_container_free(container);
						return NULL;
					}
				}
				break;
			}
			case 0xE1: {
				switch (structure->id) {
					case 0xFF: {
						structure->data = (void *) esg_session_partition_declaration_decode(buffer + structure->ptr, structure->length);
						break;
					}
					default: {
						esg_container_free(container);
						return NULL;
					}
				}
				break;
			}
			case 0xE2: {
				switch (structure->id) {
					case 0x00: {
						structure->data = (void *) esg_init_message_decode(buffer + structure->ptr, structure->length);
						break;
					}
					default: {
						esg_container_free(container);
						return NULL;
					}
				}
				break;
			}
			default: {
				esg_container_free(container);
				return NULL;
			}
		}
	}

	// Container structure body
	container->structure_body_ptr = pos;
	container->structure_body_length = size - pos;
	container->structure_body = (uint8_t *) malloc(size - pos);
	memcpy(container->structure_body, buffer + pos, size - pos);

	return container;
}

void esg_container_free(struct esg_container *container) {
	struct esg_container_structure *structure;
	struct esg_container_structure *next_structure;

	if (container == NULL) {
		return;
    }

	if (container->header) {
		for(structure = container->header->structure_list; structure; structure = next_structure) {
			next_structure = structure->_next;
			free(structure);
		}

		free(container->header);
	}

	if (container->structure_body) {
		free(container->structure_body);
	}

	free(container);
}
