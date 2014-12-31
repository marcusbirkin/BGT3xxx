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

#ifndef __SAA7231_DVB_H
#define __SAA7231_DVB_H

struct saa7231_dvb {
	char			*name;
	u8			adapter;
	int			vector;

	struct dvb_adapter	dvb_adapter;
	struct dvb_frontend	*fe;
	struct dvb_demux	demux;
	struct dmxdev		dmxdev;
	struct dmx_frontend	fe_hw;
	struct dmx_frontend	fe_mem;
	struct dvb_net		dvb_net;

	struct mutex		feedlock;
	struct tasklet_struct	tasklet;

	u8			feeds;

	struct saa7231_stream	*stream;

	struct saa7231_dev	*saa7231;
};

extern int saa7231_dvb_init(struct saa7231_dev *saa7231);
extern void saa7231_dvb_exit(struct saa7231_dev *saa7231);

#endif /* __SAA7231_DVB_H */
