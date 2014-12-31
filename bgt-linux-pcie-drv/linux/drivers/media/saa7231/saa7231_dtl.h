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

#ifndef __SAA7231_DTL_H
#define __SAA7231_DTL_H

#define INDEX(__addr)				((__addr & 0x1c000000) >> 26)
#define DMACH(__port)				(__port + 1)

#define TCMASK(__tc)				(__tc  << 29)
#define BUFMASK(__x)				(__x << 26)
#define DMAMASK(__port)				(DMACH(__port) << 21)
#define PTA(__buf, __x)				(__buf[__x].offset)

#define DMAADDR(__tc, __port, __buf, __x)	(TCMASK(__tc) | BUFMASK(__x) | DMAMASK(__port) | PTA(__buf, __x))


struct saa7231_dtl {
	u32		module;

	u32		cur_input;
	u32		addr_prev;


	u32		x_length;
	u32		y_length;
	u32		b_length;
	u32		data_loss;
	u32		wr_error;
	u32		tag_error;
	u32		dtl_halt;


	unsigned int	stream_run: 1;
	unsigned int	clock_run : 1;
	unsigned int	sync_activ: 1;
	unsigned int	stream_ena: 1;
};

#endif /* __SAA7231_DTL_H */
