/*
	ZAP utility DVB functions

	Copyright (C) 2004, 2005 Manu Abraham <abraham.manu@gmail.com>
	Copyright (C) 2006 Andrew de Quincey (adq_dvb@lidskialf.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation; either version 2.1 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#ifndef ZAP_DVB_H
#define ZAP_DVB_H 1

#include <libdvbcfg/dvbcfg_zapchannel.h>
#include <libdvbsec/dvbsec_api.h>

struct zap_dvb_params {
	int adapter_id;
	int frontend_id;
	int demux_id;
	struct dvbcfg_zapchannel channel;
	struct dvbsec_config sec;
	int valid_sec;
	struct dvbfe_handle *fe;
};

extern int zap_dvb_start(struct zap_dvb_params *params);
extern void zap_dvb_stop(void);

#endif
