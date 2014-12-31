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

#ifndef gnutv_DATA_H
#define gnutv_DATA_H 1

#include <netdb.h>

extern void gnutv_data_start(int output_type,
			   int ffaudiofd, int adapter_id, int demux_id, int buffer_size,
			   char *outfile,
			   char* outif, struct addrinfo *outaddrs, int usertp);
extern void gnutv_data_stop(void);

extern void gnutv_data_new_pat(int pmt_pid);
extern int gnutv_data_new_pmt(struct mpeg_pmt_section *pmt);



#endif
