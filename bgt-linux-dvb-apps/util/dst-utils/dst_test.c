/*
	DST-TEST utility
	an implementation for the High Level Common Interface

	Copyright (C) 2004, 2005 Manu Abraham <abraham.manu@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation; either version 2.1 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdint.h>

#include <linux/dvb/dmx.h>
#include <linux/dvb/ca.h>
#include <libdvben50221/en50221_app_tags.h>

#define CA_NODE "/dev/dvb/adapter0/ca0"

static int dst_comms(int cafd, uint32_t tag, uint32_t function, struct ca_msg *msg)
{
	if (tag) {
		msg->msg[2] = tag & 0xff;
		msg->msg[1] = (tag & 0xff00) >> 8;
		msg->msg[0] = (tag & 0xff0000) >> 16;

		printf("%s: Msg=[%02x %02x %02x ]\n",__FUNCTION__, msg->msg[0], msg->msg[1], msg->msg[2]);
	}

	if ((ioctl(cafd, function, msg)) < 0) {
		printf("%s: ioctl failed ..\n", __FUNCTION__);
		return -1;
	}

	return 0;
}


static int dst_get_caps(int cafd, struct ca_caps *caps)
{
	if ((ioctl(cafd, CA_GET_CAP, caps)) < 0) {
		printf("%s: ioctl failed ..\n", __FUNCTION__);
		return -1;
	}

	if (caps->slot_num < 1) {
		printf ("No CI Slots found\n");
		return -1;
	}

	printf("APP: Slots=[%d]\n", caps->slot_num);
	printf("APP: Type=[%d]\n", caps->slot_type);
	printf("APP: Descrambler keys=[%d]\n", caps->descr_num);
	printf("APP: Type=[%d]\n", caps->descr_type);

	return 0;
}

static int dst_get_info(int cafd, struct ca_slot_info *info)
{
	if ((ioctl(cafd, CA_GET_SLOT_INFO, info)) < 0) {
		printf("%s: ioctl failed ..\n", __FUNCTION__);
		return -1;
	}
	if (info->num < 1) {
		printf("No CI Slots found\n");
		return -1;
	}
	printf("APP: Number=[%d]\n", info->num);
	printf("APP: Type=[%d]\n", info->type);
	printf("APP: flags=[%d]\n", info->flags);

	if (info->flags == 1)
		printf("APP: CI High level interface\n");
	if (info->flags == 1)
		printf("APP: CA/CI Module Present\n");
	else if (info->flags == 2)
		printf("APP: CA/CI Module Ready\n");
	else if (info->flags == 0)
		printf("APP: No CA/CI Module\n");

	return 0;
}

static int dst_reset(int cafd)
{
	if ((ioctl(cafd, CA_RESET)) < 0) {
		printf("%s: ioctl failed ..\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

static int dst_set_pid(int cafd)
{
	if ((ioctl(cafd, CA_SET_PID)) < 0) {
		printf("%s: ioctl failed ..\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

static int dst_get_descr(int cafd)
{
	if ((ioctl(cafd, CA_GET_DESCR_INFO)) < 0) {
		printf("%s: ioctl failed ..\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

static int dst_set_descr(int cafd)
{
	if ((ioctl(cafd, CA_SET_DESCR)) < 0) {
		printf("%s: ioctl failed ..\n", __FUNCTION__);
		return -1;
	}
	return 0;
}

static int dst_get_app_info(int cafd, struct ca_msg *msg)
{
	uint32_t tag = 0;

	/*	Enquire		*/
	tag = TAG_CA_INFO_ENQUIRY;
	if ((dst_comms(cafd, tag, CA_SEND_MSG, msg)) < 0) {
		printf("%s: Dst communication failed\n", __FUNCTION__);
		return -1;
	}

	/*	Receive		*/
	tag = TAG_CA_INFO;
	if ((dst_comms(cafd, tag, CA_GET_MSG, msg)) < 0) {
		printf("%s: Dst communication failed\n", __FUNCTION__);
		return -1;
	}

	/*	Process		*/
	printf("%s: ================================ CI Module Application Info ======================================\n", __FUNCTION__);
	printf("%s: Application Type=[%d], Application Vendor=[%d], Vendor Code=[%d]\n%s: Application info=[%s]\n",
			__FUNCTION__, msg->msg[7], (msg->msg[8] << 8) | msg->msg[9], (msg->msg[10] << 8) | msg->msg[11], __FUNCTION__,
			((char *) (&msg->msg[12])));
	printf("%s: ==================================================================================================\n", __FUNCTION__);

	return 0;
}

static int dst_session_test(int cafd, struct ca_msg *msg)
{
	msg->msg[0] = 0x91;
	printf("Debugging open session request\n");
	if ((ioctl(cafd, CA_SEND_MSG, msg)) < 0) {
		printf("%s: ioctl failed ..\n", __FUNCTION__);
		return -1;
	}

	return 0;
}


int main(int argc, char *argv[])
{
	int cafd;
	const char *usage = " DST-TEST: Twinhan DST and clones test utility\n"
				" an implementation for the High Level Common Interface\n"
				" Copyright (C) 2004, 2005 Manu Abraham (manu@kromtek.com)\n\n"
				"\t dst_test options:\n"
				"\t -c capabilities\n"
				"\t -i info\n"
				"\t -r reset\n"
				"\t -p pid\n"
				"\t -g get descr\n"
				"\t -s set_descr\n"
				"\t -a app_info\n"
				"\t -t session test\n";


	struct ca_caps *caps;
	caps = (struct ca_caps *) malloc(sizeof (struct ca_caps));

	struct ca_slot_info *info;
	info = (struct ca_slot_info *)malloc (sizeof (struct ca_slot_info));

	struct ca_msg *msg;
	msg = (struct ca_msg *) malloc(sizeof (struct ca_msg));

	if (argc < 2)
		printf("%s\n", usage);

	if (argc > 1) {
		if ((cafd = open(CA_NODE, O_RDONLY)) < 0) {
			printf("%s: Error opening %s\n", __FUNCTION__, CA_NODE);
			return -1;
		}

		switch (getopt(argc, argv, "cirpgsat")) {
			case 'c':
				printf("%s: Capabilities\n", __FUNCTION__);
				dst_get_caps(cafd, caps);
				break;
			case 'i':
				printf("%s: Info\n", __FUNCTION__);
				dst_get_info(cafd, info);
				break;
			case 'r':
				printf("%s: Reset\n", __FUNCTION__);
				dst_reset(cafd);
				break;
			case 'p':
				printf("%s: PID\n", __FUNCTION__);
				dst_set_pid(cafd);
				break;
			case 'g':
				printf("%s: Get Desc\n", __FUNCTION__);
				dst_get_descr(cafd);
				break;
			case 's':
				printf("%s: Set Desc\n", __FUNCTION__);
				dst_set_descr(cafd);
				break;
			case 'a':
				printf("%s: App Info\n", __FUNCTION__);
				dst_get_app_info(cafd, msg);
				break;
			case 't':
				printf("%s: Session test\n", __FUNCTION__);
				dst_session_test(cafd, msg);
				break;

			break;
		}
		close(cafd);
	}
	return 0;
}
