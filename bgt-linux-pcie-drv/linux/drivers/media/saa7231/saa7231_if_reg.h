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

#ifndef __SAA7231_IF_REG_H
#define __SAA7231_IF_REG_H

#define IF_REG_ADDR_CONFIG1_1			0x0000
#define IF_REG_ADDR_CONFIG1_2			0x0004
#define IF_REG_ADDR_CONFIG1_3			0x0008
#define IF_REG_ADDR_CONFIG2_1			0x000C
#define IF_REG_ADDR_CONFIG2_2			0x0010
#define IF_REG_ADDR_CONFIG2_3			0x0014
#define IF_REG_PCI_ID				0x0018
#define IF_REG_SUBSYS				0x001C
#define IF_REG_CLASSCODE_TIE			0x0020
#define IF_REG_PCI_CONFIG			0x0024
#define IF_REG_BOOT_READY			0x002C
#define IF_REG_MMU_PT_LOCATION			0x0030
#define IF_REG_PCI_GLOBAL_CONFIG		0x0038
#define IF_REG_VERSIONING_FEATURE		0x0040
#define IF_REG_PCIE_BEH_CTRL			0x0044
#define IF_REG_PM_STATE_1			0x0050
#define IF_REG_SPARE				0x0054
#define IF_REG_DBG_SEL				0x0058
#define IF_REG_DBG_COUNT			0x005C
#define IF_REG_PM_CONFIG_1			0x0060
#define IF_REG_PM_CONFIG_2			0x0064
#define IF_REG_PM_CONFIG_3			0x0068
#define IF_REG_PM_CSR_0				0x0100
#define IF_REG_PM_CSR_1				0x0104
#define IF_REG_PM_CSR_2				0x0108
#define IF_REG_PM_CSR_3				0x010C
#define IF_REG_PM_CSR_4				0x0110
#define IF_REG_PM_CSR_5				0x0114
#define IF_REG_PM_CSR_6				0x0118
#define IF_REG_PM_CSR_7				0x011C
#define IF_REG_SEL_CHAN_FOR_DMA_0		0x0200
#define IF_REG_SEL_CHAN_FOR_DMA_1		0x0204
#define IF_REG_SEL_CHAN_FOR_DMA_2		0x0208
#define IF_REG_SEL_CHAN_FOR_DMA_3		0x020C
#define IF_REG_SEL_CHAN_FOR_DMA_4		0x0210
#define IF_REG_SEL_CHAN_FOR_DMA_5		0x0214
#define IF_REG_SEL_CHAN_FOR_DMA_6		0x0218
#define IF_REG_SEL_CHAN_FOR_DMA_7		0x021C
#define IF_REG_SEL_CHAN_FOR_DMA_8		0x0220
#define IF_REG_SEL_CHAN_FOR_DMA_9		0x0224
#define IF_REG_SEL_CHAN_FOR_DMA10		0x0228
#define IF_REG_SEL_CHAN_FOR_DMA11		0x022C
#define IF_REG_SEL_CHAN_FOR_DMA12		0x0230
#define IF_REG_SEL_CHAN_FOR_DMA13		0x0234
#define IF_REG_SEL_CHAN_FOR_DMA14		0x0238
#define IF_REG_SEL_CHAN_FOR_DMA15		0x023C
#define IF_REG_SOFT_RST				0x0FF4
#define IF_REG_PCPCI_MODULE_ID			0x0FFC

#define SEL_CHAN_DMA_BASE			0x0200
#define SEL_CHAN_DMA(__ch)			(SEL_CHAN_DMA_BASE + (__ch * 4))

#endif /* __SAA7231_IF_REG_H */
