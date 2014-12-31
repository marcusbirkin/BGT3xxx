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

#include <libesg/encapsulation/data_repository.h>

struct esg_data_repository *esg_data_repository_decode(uint8_t *buffer, uint32_t size) {
	struct esg_data_repository *data_repository;

	if ((buffer == NULL) || (size <= 0)) {
		return NULL;
	}

	data_repository = (struct esg_data_repository *) malloc(sizeof(struct esg_data_repository));
	memset(data_repository, 0, sizeof(struct esg_data_repository));

	data_repository->length = size;
	data_repository->data = (uint8_t *) malloc(size);
	memcpy(data_repository->data, buffer, size);

	return data_repository;
}

void esg_data_repository_free(struct esg_data_repository *data_repository) {
	if (data_repository == NULL) {
		return;
	}

	if (data_repository->data) {
		free(data_repository->data);
	}

	free(data_repository);
}
