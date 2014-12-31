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

#ifndef __SAA7231_MOD_H
#define __SAA7231_MOD_H

#define CFG			0x00000000
#define DCSN			0x00100000
#define FUSERW			0x00101000
#define TIMER0			0x00102000
#define TIMER1			0x00103000
#define TIMER2			0x00104000
#define GPIO			0x00105000
#define SPI			0x00106000
#define DMA			0x00107000
#define MSI			0x00108000
#define I2C0			0x00109000
#define I2C1			0x0010a000
#define I2C2			0x0010b000
#define I2C3			0x0010c000
#define IR			0x0010d000
#define RGU			0x0010e000
#define VS2D_AVIS		0x00120000
#define VS2D_ITU		0x00121000
#define DS2D_AVIS		0x00122000
#define DS2D_ITU		0x00123000
#define AS2D_LOCAL		0x00124000
#define AS2D_EXTERN		0x00125000
#define TS2D0_DTV		0x00126000
#define TS2D1_DTV		0x00127000
#define TS2D0_EXTERN		0x00128000
#define TS2D1_EXTERN		0x00129000
#define TS2D_CAM		0x0012a000
#define STREAM			0x0012c000
#define TSOUT_PC		0x0012e000
#define TSOUT_DCSN		0x0012f000
#define IF_REGS			0x00140000
#define MMU			0x00141000
#define CGU			0x00180000

#define AVIS			0x00200000

#define SWANLORE		0x00201000
#define COMBDEC			0x00202240
#define COMPONENT		0x00202400
#define HDSYNC			0x00202380
#define HISTOGRAM		0x00202500
#define FEATPROC		0x00202600
#define VBISLICER		0x002030C0
#define DIGOUT			0x00203340
#define DIGIF			0x00203600
#define HDSYNC_PRO		0x00203800

#define AUDIO			0x00280000
#define XRAM			0x00280000
#define DCSC			0x002ff000

#endif /* __SAA7231_MOD_H */
