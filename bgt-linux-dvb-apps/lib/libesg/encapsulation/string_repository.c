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

#include <libesg/encapsulation/string_repository.h>

struct esg_string_repository *esg_string_repository_decode(uint8_t *buffer, uint32_t size) {
	struct esg_string_repository *string_repository;

	if ((buffer == NULL) || (size <= 1)) {
		return NULL;
	}

	string_repository = (struct esg_string_repository *) malloc(sizeof(struct esg_string_repository));
	memset(string_repository, 0, sizeof(struct esg_string_repository));

	string_repository->encoding_type = buffer[0];
	string_repository->length = size-1;
	string_repository->data = (uint8_t *) malloc(size-1);
	memcpy(string_repository->data, buffer+1, size-1);

	return string_repository;
}

void esg_string_repository_free(struct esg_string_repository *string_repository) {
	if (string_repository == NULL) {
		return;
	}

	if (string_repository->data) {
		free(string_repository->data);
	}

	free(string_repository);
}
