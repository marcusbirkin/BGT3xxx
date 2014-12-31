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

#ifndef __SAA7231_S2D_REG_H
#define __SAA7231_S2D_REG_H

#define S2D_S2D_CTRL				0x0000
#define S2D_CFG_CTRL				0x0004
#define S2D_S2D_MAX_B_START			0x0008
#define S2D_CLK_GATING				0x000C

#define S2D_CHxBLOCK_OFFSET			0x0100

#define S2D_CHx_BASE(__ch)			(S2D_CHxBLOCK_OFFSET * __ch)

#define S2D_CHx_CTRL(__ch)			(S2D_CHx_BASE(__ch) + 0x0010)
#define OFFST_S2D_CHx_CTRL_WTAC			8
#define WIDTH_S2D_CHx_CTRL_WTAC			2
#define OFFST_S2D_CHx_CTRL_YJUMP		7
#define WIDTH_S2D_CHx_CTRL_YJUMP		1
#define OFFST_S2D_CHx_CTRL_SWAP			5
#define WIDTH_S2D_CHx_CTRL_SWAP			2
#define OFFST_S2D_CHx_CTRL_BMODE		4
#define WIDTH_S2D_CHx_CTRL_BMODE		1
#define OFFST_S2D_CHx_CTRL_YMODE		3
#define WIDTH_S2D_CHx_CTRL_YMODE		1
#define OFFST_S2D_CHx_CTRL_ADDRMODE		2
#define WIDTH_S2D_CHx_CTRL_ADDRMODE		1
#define OFFST_S2D_CHx_CTRL_DMADONESEL		1
#define WIDTH_S2D_CHx_CTRL_DMADONESEL		1
#define OFFST_S2D_CHx_CTRL_CHEN			0
#define WIDTH_S2D_CHx_CTRL_CHEN			1


#define S2D_CHx_B0_CTRL(__ch)			(S2D_CHx_BASE(__ch) + 0x0020)
#define OFFST_S2D_CHx_B0_CTRL_DFOT		1
#define WIDTH_S2D_CHx_B0_CTRL_DFOT		1
#define OFFST_S2D_CHx_B0_CTRL_BTAG		0
#define WIDTH_S2D_CHx_B0_CTRL_BTAG		1

#define S2D_CHx_B0_B_START_ADDRESS(__ch)	(S2D_CHx_BASE(__ch) + 0x0024)
#define S2D_CHx_B0_B_LENGTH(__ch)		(S2D_CHx_BASE(__ch) + 0x0028)
#define S2D_CHx_B0_STRIDE(__ch)			(S2D_CHx_BASE(__ch) + 0x002C)
#define S2D_CHx_B0_X_LENGTH(__ch)		(S2D_CHx_BASE(__ch) + 0x0030)
#define S2D_CHx_B0_Y_LENGTH(__ch)		(S2D_CHx_BASE(__ch) + 0x0034)

#define S2D_CHx_B1_CTRL(__ch)			(S2D_CHx_BASE(__ch) + 0x0040)
#define S2D_CHx_B1_B_START_ADDRESS(__ch)	(S2D_CHx_BASE(__ch) + 0x0044)
#define S2D_CHx_B1_B_LENGTH(__ch)		(S2D_CHx_BASE(__ch) + 0x0048)
#define S2D_CHx_B1_STRIDE(__ch)			(S2D_CHx_BASE(__ch) + 0x004C)
#define S2D_CHx_B1_X_LENGTH(__ch)		(S2D_CHx_BASE(__ch) + 0x0050)
#define S2D_CHx_B1_Y_LENGTH(__ch)		(S2D_CHx_BASE(__ch) + 0x0054)

#define S2D_CHx_B2_B_START_ADDRESS(__ch)	(S2D_CHx_BASE(__ch) + 0x0088)
#define S2D_CHx_B3_B_START_ADDRESS(__ch)	(S2D_CHx_BASE(__ch) + 0x008C)
#define S2D_CHx_B4_B_START_ADDRESS(__ch)	(S2D_CHx_BASE(__ch) + 0x0090)
#define S2D_CHx_B5_B_START_ADDRESS(__ch)	(S2D_CHx_BASE(__ch) + 0x0094)
#define S2D_CHx_B6_B_START_ADDRESS(__ch)	(S2D_CHx_BASE(__ch) + 0x0098)
#define S2D_CHx_B7_B_START_ADDRESS(__ch)	(S2D_CHx_BASE(__ch) + 0x009C)

#define S2D_CHx_CURRENT_ADDRESS(__ch)		(S2D_CHx_BASE(__ch) + 0x00A0)
#define S2D_CHx_STATUS(__ch)			(S2D_CHx_BASE(__ch) + 0x00A4)
#define S2D_CHx_ADDRESS_MASK(__ch)		(S2D_CHx_BASE(__ch) + 0x00B0)
#define S2D_CHx_ADDRESS_OVERWRITE(__ch)		(S2D_CHx_BASE(__ch) + 0x00B4)

#define S2D_S2D_TIME_STAMP			0x0400
#define S2D_S2D_STATUS				0x0404
#define S2D_CURRENT_TIME			0x0410

#define S2D_INTBLOCK_BASE			0x0fd8
#define S2D_INTBLOCK_OFFSET			0x0018
#define S2D_INT_BASE(__ch)	\
	(S2D_INTBLOCK_BASE - (S2D_INTBLOCK_OFFSET * __ch))

#define S2D_INT_CLR_ENABLE(__ch)		(S2D_INT_BASE(__ch) + 0x0000)
#define S2D_INT_SET_ENABLE(__ch)		(S2D_INT_BASE(__ch) + 0x0004)
#define S2D_INT_STATUS(__ch)			(S2D_INT_BASE(__ch) + 0x0008)
#define S2D_INT_ENABLE(__ch)			(S2D_INT_BASE(__ch) + 0x000c)
#define S2D_INT_CLR_STATUS(__ch)		(S2D_INT_BASE(__ch) + 0x0010)
#define S2D_INT_SET_STATUS(__ch)		(S2D_INT_BASE(__ch) + 0x0014)
#define OFFST_S2D_INPUT_OVFLW			13
#define WIDTH_S2D_INPUT_OVFLW			1
#define OFFST_S2D_START_INT			11
#define WIDTH_S2D_START_INT			2
#define OFFST_S2D_DMAB_DONE			9
#define WIDTH_S2D_DMAB_DONE			2
#define OFFST_S2D_DTL_HALT			8
#define WIDTH_S2D_DTL_HALT			1
#define OFFST_S2D_ERR_TAG			7
#define WIDTH_S2D_ERR_TAG			1
#define OFFST_S2D_ERR_WR			6
#define WIDTH_S2D_ERR_WR			1
#define OFFST_S2D_DATA_LOSS			5
#define WIDTH_S2D_DATA_LOSS			1
#define OFFST_S2D_B_LENGTH			4
#define WIDTH_S2D_B_LENGTH			1
#define OFFST_S2D_Y_LENGTH			3
#define WIDTH_S2D_Y_LENGTH			1
#define OFFST_S2D_X_LENGTH			2
#define WIDTH_S2D_X_LENGTH			1
#define OFFST_S2D_TAGDONE			1
#define WIDTH_S2D_TAGDONE			1
#define OFFST_S2D_DMADONE			0
#define WIDTH_S2D_DMADONE			1

#define S2D_SW_RST				0x0FF0
#define S2D_DISABLE_IF				0x0FF4
#define S2D_MODULE_ID_EXT			0x0FF8
#define S2D_MODULE_ID				0x0FFC

#endif /* __SAA7231_S2D_REG_H */
