/*
 * dvbnet.c
 *
 * Copyright (C) 2003 TV Files S.p.A
 *                    L.Y.Mesentsev <lymes@tiscalinet.it>
 *
 * Ported to use new DVB libraries:
 * Copyright (C) 2006 Andrew de Quincey <adq_dvb@lidskialf.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <libdvbapi/dvbnet.h>

#define OK    0
#define FAIL -1


enum Mode {
	UNKNOWN,
	LST_INTERFACE,
	ADD_INTERFACE,
	DEL_INTERFACE
} op_mode;

static int adapter = 0;
static int netdev = 0;

static void hello(void);
static void usage(char *);
static void parse_args(int, char **);
static void queryInterface(int);

int ifnum;
int pid;
int encapsulation;

int main(int argc, char **argv)
{
	int fd_net;

	hello();

	parse_args(argc, argv);

	if ((fd_net = dvbnet_open(adapter, netdev)) < 0) {
		fprintf(stderr, "Error: couldn't open device %d: %d %m\n",
			netdev, errno);
		return FAIL;
	}

	switch (op_mode) {
	case DEL_INTERFACE:
		if (dvbnet_remove_interface(fd_net, ifnum))
			fprintf(stderr,
				"Error: couldn't remove interface %d: %d %m.\n",
				ifnum, errno);
		else
			printf("Status: device %d removed successfully.\n",
			       ifnum);
		break;

	case ADD_INTERFACE:
		if ((ifnum = dvbnet_add_interface(fd_net, pid, encapsulation)) < 0)
			fprintf(stderr,
				"Error: couldn't add interface for pid %d: %d %m.\n",
				pid, errno);
		else
			printf
			    ("Status: device dvb%d_%d for pid %d created successfully.\n",
			     adapter, ifnum, pid);
		break;

	case LST_INTERFACE:
		queryInterface(fd_net);
		break;

	default:
		usage(argv[0]);
		return FAIL;
	}

	close(fd_net);
	return OK;
}


void queryInterface(int fd_net)
{
	int IF, nIFaces = 0;
	char *encap;

	printf("Query DVB network interfaces:\n");
	printf("-----------------------------\n");
	for (IF = 0; IF < DVBNET_MAX_INTERFACES; IF++) {
		uint16_t _pid;
		enum dvbnet_encap _encapsulation;
		if (dvbnet_get_interface(fd_net, IF, &_pid, &_encapsulation))
			continue;

		encap = "???";
		switch(_encapsulation) {
		case DVBNET_ENCAP_MPE:
			encap = "MPE";
			break;
		case DVBNET_ENCAP_ULE:
			encap = "ULE";
			break;
		}

		printf("Found device %d: interface dvb%d_%d, "
		       "listening on PID %d, encapsulation %s\n",
		       IF, adapter, IF, _pid, encap);

		nIFaces++;
	}

	printf("-----------------------------\n");
	printf("Found %d interface(s).\n\n", nIFaces);
}


void parse_args(int argc, char **argv)
{
	int c;
	char *s;
	op_mode = UNKNOWN;
	encapsulation = DVBNET_ENCAP_MPE;
	while ((c = getopt(argc, argv, "a:n:p:d:lUvh")) != EOF) {
		switch (c) {
		case 'a':
			adapter = strtol(optarg, NULL, 0);
			break;
		case 'n':
			netdev = strtol(optarg, NULL, 0);
			break;
		case 'p':
			pid = strtol(optarg, NULL, 0);
			op_mode = ADD_INTERFACE;
			break;
		case 'd':
			ifnum = strtol(optarg, NULL, 0);
			op_mode = DEL_INTERFACE;
			break;
		case 'l':
			op_mode = LST_INTERFACE;
			break;
		case 'U':
			encapsulation = DVBNET_ENCAP_ULE;
			break;
		case 'v':
			exit(OK);
		case 'h':
		default:
			s = strrchr(argv[0], '/') + 1;
			usage((s) ? s : argv[0]);
			exit(FAIL);
		}
	}
}


void usage(char *prog_name)
{
	fprintf(stderr, "Usage: %s [options]\n", prog_name);
	fprintf(stderr, "Where options are:\n");
	fprintf(stderr, "\t-a AD  : Adapter card (default 0)\n");
	fprintf(stderr, "\t-n DD  : Demux (default 0)\n");
	fprintf(stderr, "\t-p PID : Add interface listening on PID\n");
	fprintf(stderr, "\t-d NUM : Remove interface NUM\n");
	fprintf(stderr,	"\t-l     : List currently available interfaces\n");
	fprintf(stderr, "\t-U     : use ULE framing (default: MPE)\n" );
	fprintf(stderr, "\t-v     : Print current version\n\n");
}


void hello(void)
{
	printf("\nDVB Network Interface Manager\n");
	printf("Copyright (C) 2003, TV Files S.p.A\n\n");
}
