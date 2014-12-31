/*
 * test_switch.c - Test program for new API
 *
 * Copyright (C) 2001 Ralph  Metzler <ralph@convergence.de>
 *                  & Marcus Metzler <marcus@convergence.de>
                      for convergence integrated media GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/poll.h>

#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend_old.h>
#include <linux/dvb/sec.h>
#include <linux/dvb/video.h>

int fd_frontend;
int fd_sec;
int fd_demuxa;
int fd_demuxv;
int fd_demuxtt;

struct dmx_sct_filter_params sctFilterParams;

struct secCommand scmd;
struct secCmdSequence scmds;
struct dmx_pes_filter_params pesFilterParams;
struct dmx_pes_filter_params pesFilterParamsV;
struct dmx_pes_filter_params pesFilterParamsA;
struct dmx_pes_filter_params pesFilterParamsTT;
FrontendParameters frp;
int front_type;


set_front(void)
{
	fe_status_t stat = 0;
	int i, freq;
	uint32_t soff;

	scmds.miniCommand = SEC_MINI_NONE;
	scmds.numCommands = 1;
	scmds.commands = &scmd;

	soff = frp.u.qpsk.SymbolRate/16000;

	if (ioctl(fd_sec, SEC_SEND_SEQUENCE, &scmds) < 0)
		perror("setfront sec");
	usleep(100000);

	freq = frp.Frequency;

	while(tune_it(&frp));
}

set_diseqc_nb(int nr)
{
	scmd.type=0;
	scmd.u.diseqc.addr = 0x10;
	scmd.u.diseqc.cmd = 0x38;
	scmd.u.diseqc.numParams = 1;
	scmd.u.diseqc.params[0] = 0xF0 | ((nr * 4) & 0x0F) |
	  (scmds.continuousTone == SEC_TONE_ON ? 1 : 0) |
	  (scmds.voltage==SEC_VOLTAGE_18 ? 2 : 0);
}

set_ttpid(ushort ttpid)
{
        if (ttpid==0 || ttpid==0xffff || ttpid==0x1fff) {
	        ioctl(fd_demuxtt, DMX_STOP, 0);
	        return;
	}

	pesFilterParamsTT.pid     = ttpid;
	pesFilterParamsTT.input   = DMX_IN_FRONTEND;
	pesFilterParamsTT.output  = DMX_OUT_DECODER;
	pesFilterParamsTT.pes_type = DMX_PES_TELETEXT;
	pesFilterParamsTT.flags   = DMX_IMMEDIATE_START;
	if (ioctl(fd_demuxtt, DMX_SET_PES_FILTER,
		  &pesFilterParamsTT) < 0) {
	  printf("PID=%04x\n", ttpid);
		perror("set_ttpid");
	}
}

set_vpid(ushort vpid)
{
        if (vpid==0 || vpid==0xffff || vpid==0x1fff) {
	        ioctl(fd_demuxv, DMX_STOP, 0);
	        return;
	}

	pesFilterParamsV.pid     = vpid;
	pesFilterParamsV.input   = DMX_IN_FRONTEND;
	pesFilterParamsV.output  = DMX_OUT_DECODER;
	pesFilterParamsV.pes_type = DMX_PES_VIDEO;
	pesFilterParamsV.flags   = DMX_IMMEDIATE_START;
	if (ioctl(fd_demuxv, DMX_SET_PES_FILTER,
		  &pesFilterParamsV) < 0)
		perror("set_vpid");
}

set_apid(ushort apid)
{
        if (apid==0 || apid==0xffff || apid==0x1fff) {
	        ioctl(fd_demuxa, DMX_STOP, apid);
	        return;
	}
	pesFilterParamsA.pid = apid;
	pesFilterParamsA.input = DMX_IN_FRONTEND;
	pesFilterParamsA.output = DMX_OUT_DECODER;
	pesFilterParamsA.pes_type = DMX_PES_AUDIO;
	pesFilterParamsA.flags = DMX_IMMEDIATE_START;
	if (ioctl(fd_demuxa, DMX_SET_PES_FILTER,
		  &pesFilterParamsA) < 0)
		perror("set_apid");
}

int tune_it(FrontendParameters *frp)
{
	fe_status_t stat = 0;
	int res;
	FrontendEvent event;
	int count = 0;
	struct pollfd pfd[1];

	if (ioctl(fd_frontend, FE_SET_FRONTEND, frp) <0)
		perror("setfront front");



	pfd[0].fd = fd_frontend;
	pfd[0].events = POLLIN;

	if (poll(pfd,1,3000)){
		if (pfd[0].revents & POLLIN){
			printf("Getting QPSK event\n");
			if ( ioctl(fd_frontend, FE_GET_EVENT, &event)

			     == -EOVERFLOW){
				perror("qpsk get event");
				return -1;
			}
			printf("Received ");
			switch(event.type){
			case FE_UNEXPECTED_EV:
				printf("unexpected event\n");
				return -1;
			case FE_FAILURE_EV:
				printf("failure event\n");
				return -1;

			case FE_COMPLETION_EV:
				printf("completion event\n");
			}
		}
	}
	return 0;
}

set_tp(uint *freq, int ttk, int pol, uint srate, int dis)
{
	if (*freq < 11700000) {
		frp.Frequency = (*freq - 9750000);
		scmds.continuousTone = SEC_TONE_OFF;
	} else {
		frp.Frequency = (*freq - 10600000);
		scmds.continuousTone = SEC_TONE_ON;
	}
	if (pol) scmds.voltage = SEC_VOLTAGE_18;
	else scmds.voltage = SEC_VOLTAGE_13;
	set_diseqc_nb(dis);
	frp.u.qpsk.SymbolRate = srate;
	frp.u.qpsk.FEC_inner = 0;
}

get_front(void)
{
	set_vpid(0);
	set_apid(0);
	set_ttpid(0);
	scmds.voltage = SEC_VOLTAGE_18;
	scmds.miniCommand = SEC_MINI_NONE;
	scmds.continuousTone = SEC_TONE_OFF;
	scmds.numCommands=1;
	scmds.commands=&scmd;

	frp.Frequency=(12073000-10600000);
	frp.u.qpsk.SymbolRate=25378000;
	frp.u.qpsk.FEC_inner=(fe_code_rate_t)5;
}


void get_sect(int fd)
{
        u_char sec[4096];
        int len, i;
        uint16_t cpid = 0;
        uint16_t length;
        struct pollfd pfd;

        pfd.fd = fd;
        pfd.events = POLLIN;

	while (1){
		if (poll(&pfd, 1, 5000) != POLLIN) {
			printf("not found\n");
			printf("Timeout\n");
		} else {
			len = read(fd, sec, 4096);

			length  = (sec[1]& 0x0F)<<8;
			length |= (sec[2]& 0xFF);


			for (i= 0; i < length+3; i++) {
				printf("0x%02x ",sec[i]);
				if ( !((i+1)%8))
					printf("\n");
			}
			printf("\n");
		}
	}
}

int FEReadStatus(int fd, fe_status_t *stat)
{
	int ans;

	if ((ans = ioctl(fd,FE_READ_STATUS,stat)) < 0) {
		perror("FE READ STATUS: ");
		return -1;
	}

	if (*stat & FE_HAS_POWER)
		printf("FE HAS POWER\n");

	if (*stat & FE_HAS_SIGNAL)
		printf("FE HAS SIGNAL\n");

	if (*stat & FE_SPECTRUM_INV)
		printf("QPSK SPEKTRUM INV\n");

	return 0;
}

int has_signal()
{
	fe_status_t stat;

	FEReadStatus(fd_frontend, &stat);
	if (stat & FE_HAS_SIGNAL)
		return 1;
	else {
		printf("Tuning failed\n");
		return 0;
	}
}

main()
{
	int freq;

	fd_demuxtt = open("/dev/ost/demux", O_RDWR|O_NONBLOCK);
	fd_frontend = open("/dev/ost/frontend", O_RDWR);
	fd_sec = open("/dev/ost/sec", O_RDWR);
	fd_demuxa = open("/dev/ost/demux", O_RDWR|O_NONBLOCK);
	fd_demuxv = open("/dev/ost/demux", O_RDWR|O_NONBLOCK);

//	get_front();
//        set_vpid(0x7d0);
//       set_apid(0x7d2);
//	set_ttpid(0);
//	freq = 12073000;
//	set_tp(&freq, 1, 1, 25378000, 3);
//	set_diseqc_nb(dis);

	scmds.voltage = SEC_VOLTAGE_18;
//	scmds.voltage = SEC_VOLTAGE_13;
	scmds.miniCommand = SEC_MINI_NONE;
	scmds.continuousTone = SEC_TONE_OFF;
	scmds.numCommands=1;
	scmds.commands=&scmd;
	frp.Frequency = (12073000 - 10600000);
//	frp.Frequency = (11975000 - 10600000);
	scmds.continuousTone = SEC_TONE_ON;
	frp.u.qpsk.SymbolRate = 25378000;
//	frp.u.qpsk.SymbolRate = 27500000;
//	frp.u.qpsk.FEC_inner = FEC_AUTO;
	frp.u.qpsk.FEC_inner = (fe_code_rate_t)5;
	scmd.type=0;
	scmd.u.diseqc.addr = 0x10;
	scmd.u.diseqc.cmd = 0x38;
	scmd.u.diseqc.numParams = 1;
	scmd.u.diseqc.params[0] = 0xF0 | ((3 * 4) & 0x0F) |
	  (scmds.continuousTone == SEC_TONE_ON ? 1 : 0) |
	  (scmds.voltage==SEC_VOLTAGE_18 ? 2 : 0);

	if (ioctl(fd_sec, SEC_SEND_SEQUENCE, &scmds) < 0)
		perror("setfront sec");

	while(tune_it(&frp));


/*
	if ((fd_demuxa=open("/dev/ost/demux", O_RDWR|O_NONBLOCK))
	    < 0){
		perror("DEMUX DEVICE: ");
		return -1;
	}
        memset(&sctFilterParams.filter, 0, sizeof(struct dmx_filter));
        sctFilterParams.pid                       = 0x1fff;
        sctFilterParams.filter.filter[0]          = 0x00;
        sctFilterParams.filter.mask[0]            = 0x00;
        sctFilterParams.timeout                   = 0;
        sctFilterParams.flags                     = DMX_IMMEDIATE_START;

        if (ioctl(fd_demuxa, DMX_SET_FILTER, &sctFilterParams) < 0)
                perror("DMX SET FILTER:");


	get_sect(fd_demuxa);
*/
	pesFilterParamsA.pid = 0x1fff;
	pesFilterParamsA.input = DMX_IN_FRONTEND;
	pesFilterParamsA.output = DMX_OUT_TS_TAP;
	pesFilterParamsA.pes_type = DMX_PES_OTHER;
	pesFilterParamsA.flags = DMX_IMMEDIATE_START;
	if (ioctl(fd_demuxa, DMX_SET_PES_FILTER,
		  &pesFilterParamsA) < 0)
		perror("set_apid");

	while(1);
}
