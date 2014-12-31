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

#ifndef __SAA7231_DCS_REG_H
#define __SAA7231_DCS_REG_H

#define DCSN_CTRL				0x0000
#define DCSN_ADDR				0x000C
#define DCSN_STAT				0x0010
#define DCSN_FEATURES				0x0040
#define DCSN_BASE0				0x0100
#define DCSN_INT_CLR_ENABLE			0x0FD8
#define DCSN_INT_SET_ENABLE			0x0FDC
#define DCSN_INT_STATUS				0x0FE0
#define DCSN_INT_ENABLE				0x0FE4
#define DCSN_INT_CLR_STATUS			0x0FE8
#define DCSN_INT_SET_STATUS			0x0FEC
#define DCSN_MODULE_ID				0x0FFC

#endif /* __SAA7231_DCS_REG_H */
