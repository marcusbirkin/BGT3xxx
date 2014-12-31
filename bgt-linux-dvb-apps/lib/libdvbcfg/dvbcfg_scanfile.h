/*
 * dvbcfg - support for linuxtv configuration files
 * scan channel file support
 *
 * Copyright (C) 2006 Andrew de Quincey <adq_dvb@lidskialf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef DVBCFG_SCANFILE_H
#define DVBCFG_SCANFILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libdvbapi/dvbfe.h>
#include <stdio.h>

struct dvbcfg_scanfile {
	enum dvbfe_type fe_type;
	struct dvbfe_parameters fe_params;
	char polarization; /* l,r,v,h - only used for dvb-s */
};

/**
 * Callback used in dvbcfg_scanfile_parse()
 *
 * @param channel Selected channel
 * @param private_data Private data for the callback
 * @return 0 to continue, other values to stop (values > 0 are forwarded; see below)
 */
typedef int (*dvbcfg_scancallback)(struct dvbcfg_scanfile *channel, void *private_data);

/**
 * Parse a linuxtv scan file
 *
 * @param file Linuxtv scan file
 * @param callback Callback called for each scan entry
 * @param private_data Private data for the callback
 * @return on success 0 or value from the callback if it's > 0, error code on failure
 */
extern int dvbcfg_scanfile_parse(FILE *file, dvbcfg_scancallback callback, void *private_data);

#ifdef __cplusplus
}
#endif

#endif /* DVBCFG_SCANFILE_H */
