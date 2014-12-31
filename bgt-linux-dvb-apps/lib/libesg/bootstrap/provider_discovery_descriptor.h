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

#ifndef _ESG_BOOTSTRAP_PROVIDER_DISCOVERY_DESCRIPTOR_H
#define _ESG_BOOTSTRAP_PROVIDER_DISCOVERY_DESCRIPTOR_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * esg_provider_discovery_descriptor structure.
 */
struct esg_provider_discovery_descriptor {
	uint8_t *xml;
	uint32_t size;
};

/**
 * Process an esg_provider_discovery_descriptor.
 *
 * @param buffer Binary buffer to decode.
 * @param size Binary buffer size.
 * @return Pointer to an esg_provider_discovery_descriptor structure, or NULL on error.
 */
extern struct esg_provider_discovery_descriptor *esg_esg_provider_discovery_descriptor_decode(uint8_t *buffer, uint32_t size);

/**
 * Free an esg_provider_discovery_descriptor.
 *
 * @param esg Pointer to an esg_provider_discovery_descriptor structure.
 */
extern void esg_provider_discovery_descriptor_free(struct esg_provider_discovery_descriptor *provider);

#ifdef __cplusplus
}
#endif

#endif
