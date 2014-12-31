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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dvbcfg_common.h"

int dvbcfg_parse_int(char **text, char *tokens)
{
	char *start = *text;
	char *stop = *text;
	int value;

	while (*stop != '\0') {
		if (strchr(tokens, *stop) != NULL) {
			*stop = '\0';
			stop++;
			break;
		}
		stop++;
	}

	if (sscanf(start, "%i", &value) == 1) {
		*text = stop;
		return value;
	}

	*text = NULL;
	return -1;
}

int dvbcfg_parse_char(char **text, char *tokens)
{
	char *start = *text;
	char *stop = *text;
	char value;

	while (*stop != '\0') {
		if (strchr(tokens, *stop) != NULL) {
			*stop = '\0';
			stop++;
			break;
		}
		stop++;
	}

	if (sscanf(start, "%c", &value) == 1) {
		*text = stop;
		return value;
	}

	*text = NULL;
	return -1;
}

int dvbcfg_parse_setting(char **text, char *tokens, const struct dvbcfg_setting *settings)
{
	char *start = *text;
	char *stop = *text;

	while (*stop != '\0') {
		if (strchr(tokens, *stop) != NULL) {
			*stop = '\0';
			stop++;
			break;
		}
		stop++;
	}

	while (settings->name) {
		if (strcmp(start, settings->name) == 0) {
			*text = stop;
			return settings->value;
		}
		settings++;
	}

	*text = NULL;
	return -1;
}

void dvbcfg_parse_string(char **text, char *tokens, char *dest, unsigned long size)
{
	char *start = *text;
	char *stop = *text;
	unsigned long length;

	while ((*stop != '\0') && (strchr(tokens, *stop) == NULL))
		stop++;

	length = (stop - start) + 1;

	if (length <= size) {
		if (strchr(tokens, *stop) != NULL) {
			*stop = '\0';
			*text = stop + 1;
		} else
			*text = stop;
			memcpy(dest, start, length);
			return;
	}

	*text = NULL;
	return;
}

const char *dvbcfg_lookup_setting(unsigned int setting, const struct dvbcfg_setting *settings)
{
	while (settings->name) {
		if (setting == settings->value)
			return settings->name;
		settings++;
	}

	return NULL;
}
