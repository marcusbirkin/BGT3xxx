/**
 * dvbsec testing.
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
#include <libdvbsec/dvbsec_cfg.h>

void syntax(void);

struct dvbsec_config *secconfigs = NULL;
int seccount = 0;

int secload_callback(void *private, struct dvbsec_config *sec);

int main(int argc, char *argv[])
{
        if (argc != 4) {
                syntax();
        }

	if (!strcmp(argv[1], "-sec")) {

		FILE *f = fopen(argv[2], "r");
		if (!f) {
			fprintf(stderr, "Unable to load %s\n", argv[2]);
			exit(1);
		}
		dvbsec_cfg_load(f, NULL, secload_callback);
		fclose(f);

		f = fopen(argv[3], "w");
		if (!f) {
			fprintf(stderr, "Unable to write %s\n", argv[3]);
			exit(1);
		}
		dvbsec_cfg_save(f, secconfigs, seccount);
		fclose(f);

	} else {
                syntax();
        }

        exit(0);
}

int secload_callback(void *private, struct dvbsec_config *sec)
{
	(void) private;

	struct dvbsec_config *tmp = realloc(secconfigs, (seccount+1) * sizeof(struct dvbsec_config));
	if (tmp == NULL) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	secconfigs = tmp;

	memcpy(&secconfigs[seccount++], sec, sizeof(struct dvbsec_config));

	return 0;
}

void syntax()
{
        fprintf(stderr,
                "Syntax: dvbcfg_test <-zapchannel|-sec> <input filename> <output filename>\n");
        exit(1);
}
