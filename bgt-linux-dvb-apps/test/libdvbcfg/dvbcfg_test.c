/**
 * dvbcfg testing.
 *
 * Copyright (c) 2005 by Andrew de Quincey <adq_dvb@lidskialf.net>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libdvbcfg/dvbcfg_zapchannel.h>

void syntax(void);

struct dvbcfg_zapchannel *channels = NULL;
int zapcount = 0;
int zappos = 0;

int zapload_callback(struct dvbcfg_zapchannel *channel, void *private);
int zapsave_callback(struct dvbcfg_zapchannel *channel, void *private);

int main(int argc, char *argv[])
{
        if (argc != 4) {
                syntax();
        }

	if (!strcmp(argv[1], "-zapchannel")) {

		FILE *f = fopen(argv[2], "r");
		if (!f) {
			fprintf(stderr, "Unable to load %s\n", argv[2]);
			exit(1);
		}
		dvbcfg_zapchannel_parse(f, zapload_callback, NULL);
		fclose(f);

		f = fopen(argv[3], "w");
		if (!f) {
			fprintf(stderr, "Unable to write %s\n", argv[3]);
			exit(1);
		}
		dvbcfg_zapchannel_save(f, zapsave_callback, NULL);
		fclose(f);

	} else {
                syntax();
        }

        exit(0);
}

int zapload_callback(struct dvbcfg_zapchannel *channel, void *private)
{
	(void) private;

	struct dvbcfg_zapchannel *tmp = realloc(channels, (zapcount+1) * sizeof(struct dvbcfg_zapchannel));
	if (tmp == NULL) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	channels = tmp;

	memcpy(&channels[zapcount++], channel, sizeof(struct dvbcfg_zapchannel));

	return 0;
}

int zapsave_callback(struct dvbcfg_zapchannel *channel, void *private)
{
	(void) private;

	if (zappos >= zapcount)
		return 1;

	memcpy(channel, channels + zappos, sizeof(struct dvbcfg_zapchannel));
	zappos++;

	return 0;
}

void syntax()
{
        fprintf(stderr,
                "Syntax: dvbcfg_test <-zapchannel> <input filename> <output filename>\n");
        exit(1);
}
