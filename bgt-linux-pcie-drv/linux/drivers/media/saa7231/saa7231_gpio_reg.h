/*
 *	SAA7231xx PCI/PCI Express bridge driver
 *
 *	Copyright (C) Manu Abraham <abraham.manu@gmail.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __SAA7231_GPIO_REG_H
#define __SAA7231_GPIO_REG_H


#define FB0_PINS		0x00000000
#define FB0_MODE0		0x00000010
#define FB0_MODE0_SET		0x00000014
#define FB0_MODE0_RESET		0x00000018

#define FB0_MODE1		0x00000020
#define FB0_MODE1_SET		0x00000024
#define FB0_MODE1_RESET		0x00000028

#endif /* __SAA7231_GPIO_REG_H */
