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

#ifndef _ESG_ENCAPSULATION_DATA_REPOSITORY_H
#define _ESG_ENCAPSULATION_DATA_REPOSITORY_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * esg_data_repository structure.
 */
struct esg_data_repository {
	uint32_t length;
	uint8_t *data;
};

/**
 * Process an esg_data_repository.
 *
 * @param buffer Binary buffer to decode.
 * @param size Binary buffer size.
 * @return Pointer to an esg_data_repository structure, or NULL on error.
 */
extern struct esg_data_repository *esg_data_repository_decode(uint8_t *buffer, uint32_t size);

/**
 * Free an esg_data_repository.
 *
 * @param data_repository Pointer to an esg_data_repository structure.
 */
extern void esg_data_repository_free(struct esg_data_repository *data_repository);

#ifdef __cplusplus
}
#endif

#endif
