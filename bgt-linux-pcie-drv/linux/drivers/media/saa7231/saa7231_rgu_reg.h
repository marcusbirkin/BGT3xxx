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
#ifndef __SAA7231_RGU_REG_H
#define __SAA7231_RGU_REG_H

#define RGU_RESET_CTRL0				0x0100
#define RGU_RESET_CTRL1				0x0104
#define RGU_RESET_STATUS0			0x0110
#define RGU_RESET_STATUS1			0x0114
#define RGU_RESET_STATUS2			0x0118
#define RGU_RESET_ACTIVE0			0x0150
#define RGU_RESET_ACTIVE1			0x0154
#define RGU_GLOBAL_CFG0				0x0200
#define RGU_GLOBAL_CFG1				0x0204
#define RGU_GLOBAL_CFG2				0x0208
#define RGU_GLOBAL_CFG3				0x020C
#define RGU_GLOBAL_CFG4				0x0210
#define RGU_GLOBAL_CFG5				0x0214
#define RGU_GLOBAL_CFG6				0x0218
#define RGU_RESET_EXT_STATUS0			0x0400
#define RGU_RESET_EXT_STATUS1			0x0404
#define RGU_RESET_EXT_STATUS2			0x0408
#define RGU_RESET_EXT_STATUS3			0x040C
#define RGU_RESET_EXT_STATUS4			0x0410
#define RGU_RESET_EXT_STATUS5			0x0414
#define RGU_RESET_EXT_STATUS6			0x0418
#define RGU_RESET_EXT_STATUS7			0x041C
#define RGU_RESET_EXT_STATUS8			0x0420
#define RGU_RESET_EXT_STATUS9			0x0424
#define RGU_RESET_EXT_STATUS10			0x0428
#define RGU_RESET_EXT_STATUS11			0x042C
#define RGU_RESET_EXT_STATUS12			0x0430
#define RGU_RESET_EXT_STATUS13			0x0434
#define RGU_RESET_EXT_STATUS14			0x0438
#define RGU_RESET_EXT_STATUS15			0x043C
#define RGU_RESET_EXT_STATUS16			0x0440
#define RGU_RESET_EXT_STATUS17			0x0444
#define RGU_RESET_EXT_STATUS18			0x0448
#define RGU_RESET_EXT_STATUS19			0x044C
#define RGU_RESET_EXT_STATUS20			0x0450
#define RGU_RESET_EXT_STATUS21			0x0454
#define RGU_RESET_EXT_STATUS22			0x0458
#define RGU_RESET_EXT_STATUS23			0x045C
#define RGU_RESET_EXT_STATUS24			0x0460
#define RGU_RESET_EXT_STATUS25			0x0464
#define RGU_RESET_EXT_STATUS26			0x0468
#define RGU_RESET_EXT_STATUS27			0x046C
#define RGU_RESET_EXT_STATUS28			0x0470
#define RGU_RESET_EXT_STATUS29			0x0474
#define RGU_RESET_EXT_STATUS30			0x0478
#define RGU_RESET_EXT_STATUS31			0x047C
#define RGU_RESET_EXT_STATUS32			0x0480
#define RGU_RESET_EXT_STATUS33			0x0484
#define RGU_RESET_EXT_STATUS34			0x0488
#define RGU_RESET_EXT_STATUS35			0x048C
#define RGU_RESET_EXT_STATUS36			0x0490
#define RGU_RESET_EXT_STATUS37			0x0494
#define RGU_RESET_EXT_STATUS38			0x0498
#define RGU_RESET_EXT_STATUS39			0x049C
#define RGU_RESET_EXT_STATUS40			0x04A0
#define RGU_RESET_EXT_STATUS41			0x04A4
#define RGU_RESET_EXT_STATUS42			0x04A8
#define RGU_RESET_EXT_STATUS43			0x04AC
#define RGU_RESET_EXT_STATUS44			0x04B0
#define RGU_RESET_EXT_STATUS45			0x04B4
#define RGU_CONFIG_DESCRIPTOR			0x0500
#define RGU_EXT_MODULE_ID			0x0FF8
#define RGU_MODULE_ID				0x0FFC

#endif /* __SAA7231_RGU_REG_H */
