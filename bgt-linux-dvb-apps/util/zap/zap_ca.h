/*
	ZAP utility CA functions

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

#ifndef ZAP_CA_H
#define ZAP_CA_H 1

struct zap_ca_params {
	int adapter_id;
	int caslot_num;
	int moveca;
};

extern void zap_ca_start(struct zap_ca_params *params);
extern void zap_ca_stop(void);

extern int zap_ca_new_pmt(struct mpeg_pmt_section *pmt);
extern void zap_ca_new_dvbtime(time_t dvb_time);

#endif
