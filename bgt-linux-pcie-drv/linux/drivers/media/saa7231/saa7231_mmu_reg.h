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

#ifndef __SAA7231_MMU_REG_H
#define __SAA7231_MMU_REG_H

#define MMU_MODE				0x0000
#define LIN_ADDR_MODES				0x0004

#define MMU_DMA_CONFIG_BASE			0x0080
#define MMU_DMA_CONFIG_OFFSET			0x0004
#define MMU_DMA_CONFIG(__ch)	\
	(MMU_DMA_CONFIG_BASE + (MMU_DMA_CONFIG_OFFSET * __ch))

#define MMU_SW_RST				0x0FF0
#define MMU_MODULE_ID				0x0FFC

#define MMU_PTA_BASE				0x400
#define MMU_PTA_OFFSET				0x040

#define PTA_BASE(__ch)	\
	(MMU_PTA_BASE + (MMU_PTA_OFFSET * (__ch)))

#define MMU_PTA0_LSB(__ch)			(PTA_BASE(__ch) + 0x00)
#define MMU_PTA0_MSB(__ch)			(PTA_BASE(__ch) + 0x04)
#define MMU_PTA1_LSB(__ch)			(PTA_BASE(__ch) + 0x08)
#define MMU_PTA1_MSB(__ch)			(PTA_BASE(__ch) + 0x0c)
#define MMU_PTA2_LSB(__ch)			(PTA_BASE(__ch) + 0x10)
#define MMU_PTA2_MSB(__ch)			(PTA_BASE(__ch) + 0x14)
#define MMU_PTA3_LSB(__ch)			(PTA_BASE(__ch) + 0x18)
#define MMU_PTA3_MSB(__ch)			(PTA_BASE(__ch) + 0x1c)
#define MMU_PTA4_LSB(__ch)			(PTA_BASE(__ch) + 0x20)
#define MMU_PTA4_MSB(__ch)			(PTA_BASE(__ch) + 0x24)
#define MMU_PTA5_LSB(__ch)			(PTA_BASE(__ch) + 0x28)
#define MMU_PTA5_MSB(__ch)			(PTA_BASE(__ch) + 0x2c)
#define MMU_PTA6_LSB(__ch)			(PTA_BASE(__ch) + 0x30)
#define MMU_PTA6_MSB(__ch)			(PTA_BASE(__ch) + 0x34)
#define MMU_PTA7_LSB(__ch)			(PTA_BASE(__ch) + 0x38)
#define MMU_PTA7_MSB(__ch)			(PTA_BASE(__ch) + 0x3c)
#define MMU_PTA8_LSB(__ch)			(PTA_BASE(__ch) + 0x40)
#define MMU_PTA8_MSB(__ch)			(PTA_BASE(__ch) + 0x44)
#define MMU_PTA9_LSB(__ch)			(PTA_BASE(__ch) + 0x48)
#define MMU_PTA9_MSB(__ch)			(PTA_BASE(__ch) + 0x4c)
#define MMU_PTA10_LSB(__ch)			(PTA_BASE(__ch) + 0x50)
#define MMU_PTA10_MSB(__ch)			(PTA_BASE(__ch) + 0x54)
#define MMU_PTA11_LSB(__ch)			(PTA_BASE(__ch) + 0x58)
#define MMU_PTA11_MSB(__ch)			(PTA_BASE(__ch) + 0x5c)
#define MMU_PTA12_LSB(__ch)			(PTA_BASE(__ch) + 0x60)
#define MMU_PTA12_MSB(__ch)			(PTA_BASE(__ch) + 0x64)
#define MMU_PTA13_LSB(__ch)			(PTA_BASE(__ch) + 0x68)
#define MMU_PTA13_MSB(__ch)			(PTA_BASE(__ch) + 0x6c)
#define MMU_PTA14_LSB(__ch)			(PTA_BASE(__ch) + 0x70)
#define MMU_PTA14_MSB(__ch)			(PTA_BASE(__ch) + 0x74)
#define MMU_PTA15_LSB(__ch)			(PTA_BASE(__ch) + 0x78)
#define MMU_PTA15_MSB(__ch)			(PTA_BASE(__ch) + 0x7c)

#define PTA_LSB(__mem)				((u32 ) (__mem))
#define PTA_MSB(__mem)				((u32 ) ((u64)(__mem) >> 32))

#define SAA7231_QWR(__mod, __ch, __reg, __data) {		\
	writel(__data, 	     __mod + __reg##_LSB##(__ch))	\
	writel(__data >> 32, __mod + __reg##_MSB##(__ch))	\
}

#define LSB					0
#define MSB					1
#define PTA_FLD(__fld)				((__fld == LSB) ? 0 : 4)
#define MMU_PTA(__fld, __i, __ch)		(PTA_BASE(__ch) + (__i * 8) + PTA_FLD(__fld))

#endif /* __SAA7231_MMU_REG_H */
