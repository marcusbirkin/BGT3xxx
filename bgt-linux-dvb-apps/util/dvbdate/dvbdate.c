/*

   dvbdate - a program to set the system date and time from a TDT multiplex

   Copyright (C) Laurence Culhane 2002 <dvbdate@holmes.demon.co.uk>

   Mercilessly ripped off from dvbtune, Copyright (C) Dave Chapman 2001

   Revamped by Johannes Stezenbach <js@convergence.de>
   and Michael Hunold <hunold@convergence.de>

   Ported to use the standard dvb libraries and add ATSC STT
   support Andrew de Quincey <adq_dvb@lidskialf.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
   Or, point your browser to http://www.gnu.org/copyleft/gpl.html

   Copyright (C) Laurence Culhane 2002 <dvbdate@holmes.demon.co.uk>

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <libdvbapi/dvbfe.h>
#include <libdvbapi/dvbdemux.h>
#include <libucsi/dvb/section.h>
#include <libucsi/atsc/section.h>

/* How many seconds can the system clock be out before we get warned? */
#define ALLOWABLE_DELTA 30*60

char *ProgName;
int do_print;
int do_set;
int do_force;
int do_quiet;
int timeout = 25;
int adapter = 0;

void errmsg(char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	fprintf(stderr, "%s: ", ProgName);
	vfprintf(stderr, message, ap);
	va_end(ap);
}

void usage(void)
{
	fprintf(stderr, "usage: %s [-a] [-p] [-s] [-f] [-q] [-h]\n", ProgName);
	_exit(1);
}

void help(void)
{
	fprintf(stderr,
		"\nhelp:\n"
		"%s [-a] [-p] [-s] [-f] [-q] [-h] [-t n]\n"
		"  --adapter	(adapter to use, default: 0)\n"
		"  --print	(print current time, received time and delta)\n"
		"  --set	(set the system clock to received time)\n"
		"  --force	(force the setting of the clock)\n"
		"  --quiet	(be silent)\n"
		"  --help	(display this message)\n"
		"  --timeout n	(max seconds to wait, default: 25)\n", ProgName);
	_exit(1);
}

int do_options(int arg_count, char **arg_strings)
{
	static struct option Long_Options[] = {
		{"print", 0, 0, 'p'},
		{"set", 0, 0, 's'},
		{"force", 0, 0, 'f'},
		{"quiet", 0, 0, 'q'},
		{"help", 0, 0, 'h'},
		{"timeout", 1, 0, 't'},
		{"adapter", 1, 0, 'a'},
		{0, 0, 0, 0}
	};
	int c;
	int Option_Index = 0;

	while (1) {
		c = getopt_long(arg_count, arg_strings, "a:psfqht:", Long_Options, &Option_Index);
		if (c == EOF)
			break;
		switch (c) {
		case 't':
			timeout = atoi(optarg);
			if (0 == timeout) {
				fprintf(stderr, "%s: invalid timeout value\n", ProgName);
				usage();
			}
			break;
		case 'a':
			adapter = atoi(optarg);
			break;
		case 'p':
			do_print = 1;
			break;
		case 's':
			do_set = 1;
			break;
		case 'f':
			do_force = 1;
			break;
		case 'q':
			do_quiet = 1;
			break;
		case 'h':
			help();
			break;
		case '?':
			usage();
			break;
		case 0:
/*
 * Which long option has been selected?  We only need this extra switch
 * to cope with the case of wanting to assign two long options the same
 * short character code.
 */
			printf("long option index %d\n", Option_Index);
			switch (Option_Index) {
			case 0:	/* Print */
			case 1:	/* Set */
			case 2:	/* Force */
			case 3:	/* Quiet */
			case 4:	/* Help */
			case 5:	/* timeout */
			case 6:	/* adapter */
				break;
			default:
				fprintf(stderr, "%s: unknown long option %d\n", ProgName, Option_Index);
				usage();
			}
			break;
/*
 * End of Special Long-opt handling code
 */
		default:
			fprintf(stderr, "%s: unknown getopt error - returned code %02x\n", ProgName, c);
			_exit(1);
		}
	}
	return 0;
}

/*
 * Get the next UTC date packet from the TDT section
 */
int dvb_scan_date(time_t *rx_time, unsigned int to)
{
	int tdt_fd;
	uint8_t filter[18];
	uint8_t mask[18];
	unsigned char sibuf[4096];
	int size;

	// open the demuxer
	if ((tdt_fd = dvbdemux_open_demux(adapter, 0, 0)) < 0) {
		return -1;
	}

	// create a section filter for the TDT
	memset(filter, 0, sizeof(filter));
	memset(mask, 0, sizeof(mask));
	filter[0] = stag_dvb_time_date;
	mask[0] = 0xFF;
	if (dvbdemux_set_section_filter(tdt_fd, TRANSPORT_TDT_PID, filter, mask, 1, 1)) {
		close(tdt_fd);
		return -1;
	}

	// poll for data
	struct pollfd pollfd;
	pollfd.fd = tdt_fd;
	pollfd.events = POLLIN|POLLERR|POLLPRI;
	if (poll(&pollfd, 1, to * 1000) != 1) {
		close(tdt_fd);
		return -1;
	}

	// read it
	if ((size = read(tdt_fd, sibuf, sizeof(sibuf))) < 0) {
		close(tdt_fd);
		return -1;
	}

	// parse section
	struct section *section = section_codec(sibuf, size);
	if (section == NULL) {
		close(tdt_fd);
		return -1;
	}

	// parse TDT
	struct dvb_tdt_section *tdt = dvb_tdt_section_codec(section);
	if (tdt == NULL) {
		close(tdt_fd);
		return -1;
	}

	// done
	*rx_time = dvbdate_to_unixtime(tdt->utc_time);
	close(tdt_fd);
	return 0;
}


/*
 * Get the next date packet from the STT section
 */
int atsc_scan_date(time_t *rx_time, unsigned int to)
{
	int stt_fd;
	uint8_t filter[18];
	uint8_t mask[18];
	unsigned char sibuf[4096];
	int size;

	// open the demuxer
	if ((stt_fd = dvbdemux_open_demux(adapter, 0, 0)) < 0) {
		return -1;
	}

	// create a section filter for the STT
	memset(filter, 0, sizeof(filter));
	memset(mask, 0, sizeof(mask));
	filter[0] = stag_atsc_system_time;
	mask[0] = 0xFF;
	if (dvbdemux_set_section_filter(stt_fd, ATSC_BASE_PID, filter, mask, 1, 1)) {
		close(stt_fd);
		return -1;
	}

	// poll for data
	struct pollfd pollfd;
	pollfd.fd = stt_fd;
	pollfd.events = POLLIN|POLLERR|POLLPRI;
	if (poll(&pollfd, 1, to * 1000) != 1) {
		close(stt_fd);
		return -1;
	}

	// read it
	if ((size = read(stt_fd, sibuf, sizeof(sibuf))) < 0) {
		close(stt_fd);
		return -1;
	}

	// parse section
	struct section *section = section_codec(sibuf, size);
	if (section == NULL) {
		close(stt_fd);
		return -1;
	}
	struct section_ext *section_ext = section_ext_decode(section, 0);
	if (section_ext == NULL) {
		close(stt_fd);
		return -1;
	}
	struct atsc_section_psip *psip = atsc_section_psip_decode(section_ext);
	if (psip == NULL) {
		close(stt_fd);
		return -1;
	}

	// parse STT
	struct atsc_stt_section *stt = atsc_stt_section_codec(psip);
	if (stt == NULL) {
		close(stt_fd);
		return -1;
	}

	// done
	*rx_time = atsctime_to_unixtime(stt->system_time);
	close(stt_fd);
	return 0;
}


/*
 * Set the system time
 */
int set_time(time_t * new_time)
{
	if (stime(new_time)) {
		perror("Unable to set time");
		return -1;
	}
	return 0;
}


int main(int argc, char **argv)
{
	time_t rx_time;
	time_t real_time;
	time_t offset;
	int ret;
	struct dvbfe_handle *fe;
	struct dvbfe_info fe_info;

	do_print = 0;
	do_force = 0;
	do_set = 0;
	do_quiet = 0;
	ProgName = argv[0];

/*
 * Process command line arguments
 */
	do_options(argc, argv);
	if (do_quiet && do_print) {
		errmsg("quiet and print options are mutually exclusive.\n");
		exit(1);
	}

/*
 * Find the frontend type
 */
	if ((fe = dvbfe_open(adapter, 0, 1)) == NULL) {
		errmsg("Unable to open frontend.\n");
		exit(1);
	}
	dvbfe_get_info(fe, 0, &fe_info, DVBFE_INFO_QUERYTYPE_IMMEDIATE, 0);

/*
 * Get the date from the currently tuned multiplex
 */
	switch(fe_info.type) {
	case DVBFE_TYPE_DVBS:
	case DVBFE_TYPE_DVBC:
	case DVBFE_TYPE_DVBT:
		ret = dvb_scan_date(&rx_time, timeout);
		break;

	case DVBFE_TYPE_ATSC:
		ret = atsc_scan_date(&rx_time, timeout);
		break;

	default:
		errmsg("Unsupported frontend type.\n");
		exit(1);
	}
	if (ret != 0) {
		errmsg("Unable to get time from multiplex.\n");
		exit(1);
	}
	time(&real_time);
	offset = rx_time - real_time;
	if (do_print) {
		fprintf(stdout, "System time: %s", ctime(&real_time));
		fprintf(stdout, "    RX time: %s", ctime(&rx_time));
		fprintf(stdout, "     Offset: %ld seconds\n", offset);
	} else if (!do_quiet) {
		fprintf(stdout, "%s", ctime(&rx_time));
	}
	if (do_set) {
		if (labs(offset) > ALLOWABLE_DELTA) {
			if (do_force) {
				if (0 != set_time(&rx_time)) {
					errmsg("setting the time failed\n");
				}
			} else {
				errmsg("multiplex time differs by more than %d from system.\n", ALLOWABLE_DELTA);
				errmsg("use -f to force system clock to new time.\n");
				exit(1);
			}
		} else {
			set_time(&rx_time);
		}
	}			/* #end if (do_set) */
	return (0);
}
