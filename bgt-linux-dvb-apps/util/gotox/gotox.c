/*
 *  Copyright (C) 2006 by Michel Verbraak <michel@verbraak.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the
 *  Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libdvbapi/dvbfe.h>
#include <libdvbsec/dvbsec_api.h>

static char *usage_str =
	"\nusage: gotox [options] -d <angle>\n"
	"         Goto the specified angle. Positive value for East rotation\n"
	"         Negative value for West rotation on Northern Hemisphere.\n"
	"     -d degrees : Angle to turn to in degrees (default 0)\n"
	"     -a number : use given adapter (default 0)\n"
	"     -f number : use given frontend (default 0)\n"
	"     -t seconds : leave power on to rotor for at least specified seconds of time (default 30)\n\n";

int main(int argc, char *argv[])
{
	struct dvbfe_handle *fe;
	unsigned int adapter = 0, frontend = 0;
	double angle = 0;
	int opt;
	unsigned int sleepcount = 30;
	static char *weststr = "west";
	static char *eaststr = "east";

	while ((opt = getopt(argc, argv, "ha:f:t:d:")) != -1) {

		switch (opt){
		case '?':
		case 'h':
		default:
			fprintf(stderr, "%s", usage_str);
			return EXIT_SUCCESS; 

		case 'a':
			adapter = strtoul(optarg, NULL, 0);
			break;

		case 'f':
			frontend = strtoul(optarg, NULL, 0);
			break;

		case 't':
			sleepcount = strtoul(optarg, NULL, 0);
			break;

		case 'd':
			angle = strtod(optarg, NULL);
			break;
		}
	}

	printf("Will try to rotate %s to %.2f degrees.\n", (angle < 0.0) ? weststr : eaststr, angle );

	fe = dvbfe_open(adapter, frontend, 0);
	if (fe == NULL) {
		fprintf(stderr, "Could not open frontend %d on adapter %d.\n", frontend, adapter);
		exit(1);
	}

	if (dvbfe_set_voltage(fe, DVBFE_SEC_VOLTAGE_OFF) != 0) {
		fprintf(stderr, "Could not turn off power.\n");
		dvbfe_close(fe);
		return 1;
	}
	else
		printf("Power OFF.\n");

	sleep(1);

	if (dvbfe_set_voltage(fe, DVBFE_SEC_VOLTAGE_18) != 0) {
		fprintf(stderr, "Could not turn on power.\n");
		dvbfe_close(fe);
		return 1;
	}
	else
		printf("Power on to 18V.\n");

	sleep(1);

	if (abs(angle) == 0.0) {

		if (dvbsec_diseqc_goto_satpos_preset(fe, DISEQC_ADDRESS_POLAR_AZIMUTH_POSITIONER, 0) != 0) {
			fprintf(stderr, "Could not goto 0.\n");
			dvbfe_close(fe);
			return 2;
		} else {
			printf("Going to home base 0 degrees.\n");
		}
	} else {

		if (dvbsec_diseqc_goto_rotator_bearing(fe, DISEQC_ADDRESS_POLAR_AZIMUTH_POSITIONER, angle) != 0) {
			fprintf(stderr, "Could not rotate.\n");
			dvbfe_close(fe);
			return 2;
		}
	}

	while (sleepcount != 0) {
		printf("%d: Rotating to %.2f.\r", sleepcount, angle);
		fflush(NULL);
		sleepcount--;
		sleep(1);
	}

	printf("\nRotated.\n");

	if (dvbfe_set_voltage(fe, DVBFE_SEC_VOLTAGE_OFF) != 0) {
		fprintf(stderr, "Could not turn off power.\n");
		dvbfe_close(fe);
		return 1;
	}
	else
		printf("Power OFF.\n");

	sleep(1);

	dvbfe_close(fe);

	return EXIT_SUCCESS;
}
