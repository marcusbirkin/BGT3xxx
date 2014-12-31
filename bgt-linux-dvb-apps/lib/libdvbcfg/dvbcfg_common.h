/*
 * dvbcfg - support for linuxtv configuration files
 * common functions
 *
 * Copyright (C) 2006 Christoph Pfister <christophpfister@gmail.com>
 * Copyright (C) 2005 Andrew de Quincey <adq_dvb@lidskialf.net>
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

#ifndef DVBCFG_COMMON_H
#define DVBCFG_COMMON_H 1

struct dvbcfg_setting {
	const char *name;
	unsigned int value;
};

extern int dvbcfg_parse_int(char **text, char *tokens);
extern int dvbcfg_parse_char(char **text, char *tokens);
extern int dvbcfg_parse_setting(char **text, char *tokens, const struct dvbcfg_setting *settings);
extern void dvbcfg_parse_string(char **text, char *tokens, char *dest, unsigned long size);
extern const char *dvbcfg_lookup_setting(unsigned int setting, const struct dvbcfg_setting *settings);

#endif
