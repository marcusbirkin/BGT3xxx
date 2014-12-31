/*
 * test.c - Test program for new API
 *
 * Copyright (C) 2000 Ralph  Metzler <ralph@convergence.de>
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

#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend_old.h>
#include <linux/dvb/sec.h>
#include <linux/dvb/video.h>

uint8_t reverse[] = {
0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF
};

inline t2a(uint8_t c)
{
  c=reverse[c]&0x7f;
  if (c<0x20)
    c=0x20;

  return c;
}

void testpesfilter(void)
{
        uint8_t buf[4096], id;
	int i,j;
	int len;
	int fd=open("/dev/ost/demux1", O_RDWR);
	struct dmx_pes_filter_params pesFilterParams;

	pesFilterParams.input = DMX_IN_FRONTEND;
	pesFilterParams.output = DMX_OUT_TS_TAP;
	pesFilterParams.pes_type = DMX_PES_TELETEXT;
	pesFilterParams.flags = DMX_IMMEDIATE_START;

	pesFilterParams.pid = 0x2c ;
	if (ioctl(fd, DMX_SET_PES_FILTER, &pesFilterParams) < 0) {
	        printf("Could not set PES filter\n");
		close(fd);
		return;
	}
	ioctl(fd, DMX_STOP, 0);
/*
	pesFilterParams.pid = 54;
	if (ioctl(fd, DMX_SET_PES_FILTER, &pesFilterParams) < 0) {
	        printf("Could not set PES filter\n");
		close(fd);
		return;
	}
	ioctl(fd, DMX_STOP, 0);

	pesFilterParams.pid = 55;
	if (ioctl(fd, DMX_SET_PES_FILTER, &pesFilterParams) < 0) {
	        printf("Could not set PES filter\n");
		close(fd);
		return;
	}
*/
	while(1) {
		len=read(fd, buf, 4096);
		if (len>0) write(1, buf, len);
	}

	do {
	        read(fd, buf, 4);
		if (htonl(*(uint32_t *)buf)!=0x00001bd)
		        continue;
		read(fd, buf+4, 2);
		len=(buf[4]<<8)|buf[5];
	        read(fd, buf+6, len);
		fprintf(stderr,"read %d bytes PES\n", len);
		write (1, buf, len+6);

		id=buf[45];
		fprintf(stderr,"id=%02x\n", id);

		for (i=0; i<(len+6-46)/46; i++) {
		  for (j=6; j<46; j++) {
		    fprintf(stderr,"%c", t2a(buf[i*46+46+j]));
		  }
		    fprintf(stderr,"\n");
		}

	} while (1);


	/*
	pesFilterParams.pid = 55;
	pesFilterParams.input = DMX_IN_FRONTEND;
	pesFilterParams.output = DMX_OUT_DECODER;
	pesFilterParams.pes_type = DMX_PES_TELETEXT;
	pesFilterParams.flags = DMX_IMMEDIATE_START;

	ioctl(fd, DMX_SET_PES_FILTER, &pesFilterParams);
	close(fd);
	*/
}


senf()
{
  int ret;
  int len;
  struct secCommand scmd;
  struct secCmdSequence scmds;
  struct dmx_pes_filter_params pesFilterParams;
  struct dmx_sct_filter_params secFilterParams;
  FrontendParameters frp;
  uint8_t buf[4096];

  int vout=open("qv", O_RDWR|O_CREAT);
  int aout=open("qa", O_RDWR|O_CREAT);

  int fd_video=open("/dev/ost/video", O_RDWR);
  int fd_audio=open("/dev/ost/audio", O_RDWR);
  int fd_tt=open("/dev/ost/demux", O_RDWR);
  int fd_frontend=open("/dev/ost/frontend", O_RDWR);
  int fd_sec=open("/dev/ost/sec", O_RDWR);
  int fd_demux=open("/dev/ost/demux", O_RDWR|O_NONBLOCK);
  int fd_demux2=open("/dev/ost/demux", O_RDWR|O_NONBLOCK);
  int fd_section=open("/dev/ost/demux", O_RDWR);
  int fd_section2=open("/dev/ost/demux", O_RDWR);

  printf("%d\n",fd_section);

  //if (ioctl(fd_sec, SEC_SET_VOLTAGE, SEC_VOLTAGE_13) < 0)  return;
  //if (ioctl(fd_sec, SEC_SET_TONE, SEC_TONE_ON) < 0)  return;
#if 0
  scmd.type=0;
  scmd.u.diseqc.addr=0x10;
  scmd.u.diseqc.cmd=0x38;
  scmd.u.diseqc.numParams=1;
  scmd.u.diseqc.params[0]=0xf0;

  scmds.voltage=SEC_VOLTAGE_13;
  scmds.miniCommand=SEC_MINI_NONE;
  scmds.continuousTone=SEC_TONE_ON;
  scmds.numCommands=1;
  scmds.commands=&scmd;
  if (ioctl(fd_sec, SEC_SEND_SEQUENCE, &scmds) < 0)  return;
  printf("SEC OK\n");

  frp.Frequency=(12666000-10600000);
  frp.u.qpsk.SymbolRate=22000000;
  frp.u.qpsk.FEC_inner=FEC_AUTO;

  if (ioctl(fd_frontend, FE_SET_FRONTEND, &frp) < 0)  return;
  printf("QPSK OK\n");
#endif

#if 0
  ret=ioctl(fd_demux2, DMX_SET_BUFFER_SIZE, 64*1024);
    if (ret<0)
      perror("DMX_SET_BUFFER_SIZE\n");
  printf("Audio filter size OK\n");
  pesFilterParams.pid = 0x60;
  pesFilterParams.input = DMX_IN_FRONTEND;
  pesFilterParams.output = DMX_OUT_DECODER;
  pesFilterParams.pes_type = DMX_PES_AUDIO;
  pesFilterParams.flags = DMX_IMMEDIATE_START;

  if (ioctl(fd_demux2, DMX_SET_PES_FILTER, &pesFilterParams) < 0)   return(1);
  printf("Audio filter OK\n");

  if (ioctl(fd_demux, DMX_SET_BUFFER_SIZE, 64*1024) < 0)  return(1);
  pesFilterParams.pid = 0xa2;
  pesFilterParams.input = DMX_IN_FRONTEND;
  pesFilterParams.output = DMX_OUT_DECODER;
  pesFilterParams.pes_type = DMX_PES_VIDEO;
  pesFilterParams.flags = DMX_IMMEDIATE_START;
  if (ioctl(fd_demux, DMX_SET_PES_FILTER, &pesFilterParams) < 0)  return(1);
  printf("Video filter OK\n");
#endif
  /*
  pesFilterParams.pid = 56;
  pesFilterParams.input = DMX_IN_FRONTEND;
  pesFilterParams.output = DMX_OUT_DECODER;
  pesFilterParams.pes_type = DMX_PES_TELETEXT;
  pesFilterParams.flags = DMX_IMMEDIATE_START;
  if (ioctl(fd_tt, DMX_SET_PES_FILTER, &pesFilterParams) < 0)  return(1);
  printf("TT filter OK\n");
  */
  //while (1);
  /* {
    len=read(fd_demux, buf, 4096);
    if (len>0) write (vout, buf, len);
    len=read(fd_demux2, buf, 4096);
    if (len>0) write (aout, buf, len);
    }*/


  memset(&secFilterParams.filter.filter, 0, DMX_FILTER_SIZE);
  memset(&secFilterParams.filter.mask, 0, DMX_FILTER_SIZE);
  secFilterParams.filter.filter[0]=0x00;
  secFilterParams.filter.mask[0]=0x00;
  secFilterParams.timeout=5000;
  secFilterParams.flags=DMX_IMMEDIATE_START;

  /*
  // this one should timeout after 2 seconds
  secFilterParams.pid=0xa4;
  if (ioctl(fd_section, DMX_SET_FILTER, &secFilterParams) < 0)  return;
  len=read(fd_section, buf, 4096);
  */

  /*
  {
    int32_t snr, str;
    ioctl(fd_frontend, FE_READ_SNR, &snr);
    ioctl(fd_frontend, FE_READ_SIGNAL_STRENGTH, &str);

    printf("snr=%d, str=%d\n", snr, str);
  }
  */

  secFilterParams.pid=0x11;
  //printf("section filter\n");
  if (ioctl(fd_section, DMX_SET_FILTER, &secFilterParams) < 0)  return;
  //if (ioctl(fd_section2, DMX_SET_FILTER, &secFilterParams) < 0)  return;
  //close(fd_section2);
  //while (1)
{
    len=read(fd_section, buf, 4096);
    if (len>0) write(1, buf, len);
    //printf("read section with length %d\n", len);
    //if (len>0) write(1,buf,len);
    //len=read(fd_section2, buf, 4096);
    //if (len>0) write(1,buf,len);
    //printf("read section with length %d\n", len);
  }

}

main()
{
	//senf();
  testpesfilter();
}
