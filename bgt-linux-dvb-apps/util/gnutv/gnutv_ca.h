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

#ifndef gnutv_CA_H
#define gnutv_CA_H 1

struct gnutv_ca_params {
	int adapter_id;
	int caslot_num;
	int cammenu;
	int moveca;
};

extern void gnutv_ca_start(struct gnutv_ca_params *params);
extern void gnutv_ca_ui(void);
extern void gnutv_ca_stop(void);

extern int gnutv_ca_new_pmt(struct mpeg_pmt_section *pmt);
extern void gnutv_ca_new_dvbtime(time_t dvb_time);

#endif
