/*
	gnutv utility

	Copyright (C) 2004, 2005 Manu Abraham <abraham.manu@gmail.com>
	Copyright (C) 2006 Andrew de Quincey (adq_dvb@lidskialf.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef gnutv_DVB_H
#define gnutv_DVB_H 1

#include <libdvbcfg/dvbcfg_zapchannel.h>
#include <libdvbsec/dvbsec_api.h>

struct gnutv_dvb_params {
	int adapter_id;
	int frontend_id;
	int demux_id;
	struct dvbcfg_zapchannel channel;
	struct dvbsec_config sec;
	int valid_sec;
	int output_type;
	struct dvbfe_handle *fe;
};

extern int gnutv_dvb_start(struct gnutv_dvb_params *params);
extern void gnutv_dvb_stop(void);
extern int gnutv_dvb_locked(void);

#endif
