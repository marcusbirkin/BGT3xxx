/*
	dvbscan utility

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dvbscan.h"

void append_transponder(struct transponder *t, struct transponder **tlist, struct transponder **tlist_end)
{
	if (*tlist_end == NULL) {
		*tlist = t;
	} else {
		(*tlist_end)->next = t;
	}
	*tlist_end = t;
	t->next = NULL;
}

struct transponder *new_transponder(void)
{
	struct transponder *t = (struct transponder *) malloc(sizeof(struct transponder));
	if (t == NULL) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	memset(t, 0, sizeof(struct transponder));

	return t;
}

void free_transponder(struct transponder *t)
{
	if (t->frequencies)
		free(t->frequencies);
	// FIXME: free services
	free(t);
}

int seen_transponder(struct transponder *t, struct transponder *checklist)
{
	uint32_t i;

	struct transponder *cur_check = checklist;
	while(cur_check) {
		uint32_t freq1 = cur_check->params.frequency / 2000;
		for(i=0; i < t->frequency_count; i++) {
			uint32_t freq2 = t->frequencies[i] / 2000;
			if (freq1 == freq2) {
				return 1;
			}
		}
		cur_check = cur_check->next;
	}

	return 0;
}

void add_frequency(struct transponder *t, uint32_t frequency)
{
	uint32_t *tmp;

	tmp = (uint32_t*) realloc(t->frequencies, sizeof(uint32_t) * (t->frequency_count + 1));
	if (tmp == NULL) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	tmp[t->frequency_count++] = frequency;
	t->frequencies = tmp;
}

struct transponder *first_transponder(struct transponder **tlist, struct transponder **tlist_end)
{
	struct transponder *t = *tlist;

	*tlist = t->next;
	if (*tlist == NULL)
		*tlist_end = NULL;

	return t;
}
