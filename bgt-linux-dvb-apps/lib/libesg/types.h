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

#ifndef _ESG_TYPES_H
#define _ESG_TYPES_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/**
 * esg_ip_address
 */
union esg_ip_address {
	uint8_t ipv4[4];
	uint8_t ipv6[16];
};

/**
 * Process a vluimsbf8 length.
 *
 * @param buffer Binary buffer to decode.
 * @param size Binary buffer size.
 * @param length Read length value
 * @return vluimsbf8 size
 */
extern uint8_t vluimsbf8(uint8_t *buffer, uint32_t size, uint32_t *length);

#ifdef __cplusplus
}
#endif

#endif
