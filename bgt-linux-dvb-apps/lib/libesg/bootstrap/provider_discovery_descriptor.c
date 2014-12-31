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

#include <libesg/bootstrap/provider_discovery_descriptor.h>

struct esg_provider_discovery_descriptor *esg_esg_provider_discovery_descriptor_decode(uint8_t *buffer, uint32_t size) {
	struct esg_provider_discovery_descriptor *provider;

	provider = (struct esg_provider_discovery_descriptor *) malloc(sizeof(struct esg_provider_discovery_descriptor));
	memset(provider, 0, sizeof(struct esg_provider_discovery_descriptor));

	provider->xml = (uint8_t *) malloc(size);
	memcpy(provider->xml, buffer, size);

	provider->size = size;

	return provider;
}

void esg_provider_discovery_descriptor_free(struct esg_provider_discovery_descriptor *provider) {
	if (provider == NULL) {
		return;
	}

	if (provider->xml) {
		free(provider->xml);
	}

	free(provider);
}
