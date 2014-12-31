/*
	gnutv utility

	Copyright (C) 2004, 2005 Manu Abraham <abraham.manu@gmail.com>
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

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libdvbapi/dvbdemux.h>
#include <libdvbapi/dvbaudio.h>
#include <libucsi/mpeg/section.h>
#include "gnutv.h"
#include "gnutv_dvb.h"
#include "gnutv_ca.h"
#include "gnutv_data.h"

static void *fileoutputthread_func(void* arg);
static void *udpoutputthread_func(void* arg);

static int gnutv_data_create_decoder_filter(int adapter, int demux, uint16_t pid, int pestype);
static int gnutv_data_create_dvr_filter(int adapter, int demux, uint16_t pid);

static void gnutv_data_decoder_pmt(struct mpeg_pmt_section *pmt);
static void gnutv_data_dvr_pmt(struct mpeg_pmt_section *pmt);

static void gnutv_data_append_pid_fd(int pid, int fd);
static void gnutv_data_free_pid_fds(void);

static pthread_t outputthread;
static int outfd = -1;
static int dvrfd = -1;
static int pat_fd_dvrout = -1;
static int pmt_fd_dvrout = -1;
static int outputthread_shutdown = 0;

static int usertp = 0;
static int adapter_id = -1;
static int demux_id = -1;
static int output_type = 0;
static struct addrinfo *outaddrs = NULL;

struct pid_fd {
	int pid;
	int fd;
};
static struct pid_fd *pid_fds = NULL;
static int pid_fds_count = 0;

void gnutv_data_start(int _output_type,
		    int ffaudiofd, int _adapter_id, int _demux_id, int buffer_size,
		    char *outfile,
		    char* outif, struct addrinfo *_outaddrs, int _usertp)
{
	usertp = _usertp;
	demux_id = _demux_id;
	adapter_id = _adapter_id;
	output_type = _output_type;

	// setup output
	switch(output_type) {
	case OUTPUT_TYPE_DECODER:
	case OUTPUT_TYPE_DECODER_ABYPASS:
		dvbaudio_set_bypass(ffaudiofd, (output_type == OUTPUT_TYPE_DECODER_ABYPASS) ? 1 : 0);
		close(ffaudiofd);
		break;

	case OUTPUT_TYPE_STDOUT:
	case OUTPUT_TYPE_FILE:
		if (output_type == OUTPUT_TYPE_FILE) {
			// open output file
			outfd = open(outfile, O_WRONLY|O_CREAT|O_LARGEFILE|O_TRUNC, 0644);
			if (outfd < 0) {
				fprintf(stderr, "Failed to open output file\n");
				exit(1);
			}
		} else {
			outfd = STDOUT_FILENO;
		}

		// open dvr device
		dvrfd = dvbdemux_open_dvr(adapter_id, 0, 1, 0);
		if (dvrfd < 0) {
			fprintf(stderr, "Failed to open DVR device\n");
			exit(1);
		}

		// optionally set dvr buffer size
		if (buffer_size > 0) {
			if (dvbdemux_set_buffer(dvrfd, buffer_size) != 0) {
				fprintf(stderr, "Failed to set DVR buffer size\n");
				exit(1);
			}
		}

		pthread_create(&outputthread, NULL, fileoutputthread_func, NULL);
		break;

	case OUTPUT_TYPE_UDP:
		outaddrs = _outaddrs;

		// open output socket
		outfd = socket(outaddrs->ai_family, outaddrs->ai_socktype, outaddrs->ai_protocol);
		if (outfd < 0) {
			fprintf(stderr, "Failed to open output socket\n");
			exit(1);
		}

		// bind to local interface if requested
		if (outif != NULL) {
			if (setsockopt(outfd, SOL_SOCKET, SO_BINDTODEVICE, outif, strlen(outif)) < 0) {
				fprintf(stderr, "Failed to bind to interface %s\n", outif);
				exit(1);
			}
		}

		// open dvr device
		dvrfd = dvbdemux_open_dvr(adapter_id, 0, 1, 0);
		if (dvrfd < 0) {
			fprintf(stderr, "Failed to open DVR device\n");
			exit(1);
		}

		// optionally set dvr buffer size
		if (buffer_size > 0) {
			if (dvbdemux_set_buffer(dvrfd, buffer_size) != 0) {
				fprintf(stderr, "Failed to set DVR buffer size\n");
				exit(1);
			}
		}

		pthread_create(&outputthread, NULL, udpoutputthread_func, NULL);
		break;
	}

	// output PAT to DVR if requested
	switch(output_type) {
	case OUTPUT_TYPE_DVR:
	case OUTPUT_TYPE_FILE:
	case OUTPUT_TYPE_STDOUT:
	case OUTPUT_TYPE_UDP:
		pat_fd_dvrout = gnutv_data_create_dvr_filter(adapter_id, demux_id, TRANSPORT_PAT_PID);
	}
}

void gnutv_data_stop()
{
	// shutdown output thread if necessary
	if (dvrfd != -1) {
		outputthread_shutdown = 1;
		pthread_join(outputthread, NULL);
	}
	gnutv_data_free_pid_fds();
	if (pat_fd_dvrout != -1)
		close(pat_fd_dvrout);
	if (pmt_fd_dvrout != -1)
		close(pmt_fd_dvrout);
	if (outaddrs)
		freeaddrinfo(outaddrs);
}

void gnutv_data_new_pat(int pmt_pid)
{
	// output PMT to DVR if requested
	switch(output_type) {
	case OUTPUT_TYPE_DVR:
	case OUTPUT_TYPE_FILE:
	case OUTPUT_TYPE_STDOUT:
	case OUTPUT_TYPE_UDP:
		if (pmt_fd_dvrout != -1)
			close(pmt_fd_dvrout);
		pmt_fd_dvrout = gnutv_data_create_dvr_filter(adapter_id, demux_id, pmt_pid);
	}
}

int gnutv_data_new_pmt(struct mpeg_pmt_section *pmt)
{
	// close all old PID FDs
	gnutv_data_free_pid_fds();

	// deal with the PMT appropriately
	switch(output_type) {
	case OUTPUT_TYPE_DECODER:
	case OUTPUT_TYPE_DECODER_ABYPASS:
		gnutv_data_decoder_pmt(pmt);
		break;

	case OUTPUT_TYPE_DVR:
	case OUTPUT_TYPE_FILE:
	case OUTPUT_TYPE_STDOUT:
	case OUTPUT_TYPE_UDP:
		gnutv_data_dvr_pmt(pmt);
		break;
	}

	return 1;
}

static void *fileoutputthread_func(void* arg)
{
	(void)arg;
	uint8_t buf[4096];
	struct pollfd pollfd;
	int written;

	pollfd.fd = dvrfd;
	pollfd.events = POLLIN|POLLPRI|POLLERR;

	while(!outputthread_shutdown) {
		if (poll(&pollfd, 1, 1000) == -1) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "DVR device poll failure\n");
			return 0;
		}

		if (pollfd.revents == 0)
			continue;

		int size = read(dvrfd, buf, sizeof(buf));
		if (size < 0) {
			if (errno == EINTR)
				continue;

			if (errno == EOVERFLOW) {
				// The error flag has been cleared, next read should succeed.
				fprintf(stderr, "DVR overflow\n");
				continue;
			}

			fprintf(stderr, "DVR device read failure\n");
			return 0;
		}

		written = 0;
		while(written < size) {
			int tmp = write(outfd, buf + written, size - written);
			if (tmp == -1) {
				if (errno != EINTR) {
					fprintf(stderr, "Write error: %m\n");
					break;
				}
			} else {
				written += tmp;
			}
		}
	}

	return 0;
}

#define TS_PAYLOAD_SIZE (188*7)

static void *udpoutputthread_func(void* arg)
{
	(void)arg;
	uint8_t buf[12 + TS_PAYLOAD_SIZE];
	struct pollfd pollfd;
	int bufsize = 0;
	int bufbase = 0;
	int readsize;
	uint16_t rtpseq = 0;

	pollfd.fd = dvrfd;
	pollfd.events = POLLIN|POLLPRI|POLLERR;

	if (usertp) {
		srandom(time(NULL));
		int ssrc = random();
		rtpseq = random();
		buf[0x0] = 0x80;
		buf[0x1] = 0x21;
		buf[0x4] = 0x00; // }
		buf[0x5] = 0x00; // } FIXME: should really be a valid stamp
		buf[0x6] = 0x00; // }
		buf[0x7] = 0x00; // }
		buf[0x8] = ssrc >> 24;
		buf[0x9] = ssrc >> 16;
		buf[0xa] = ssrc >> 8;
		buf[0xb] = ssrc;
		bufbase = 12;
	}

	while(!outputthread_shutdown) {
		if (poll(&pollfd, 1, 1000) != 1)
			continue;
		if (pollfd.revents & POLLERR) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "DVR device read failure\n");
			return 0;
		}

		readsize = TS_PAYLOAD_SIZE - bufsize;
		readsize = read(dvrfd, buf + bufbase + bufsize, readsize);
		if (readsize < 0) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "DVR device read failure\n");
			return 0;
		}
		bufsize += readsize;

		if (bufsize == TS_PAYLOAD_SIZE) {
			if (usertp) {
				buf[2] = rtpseq >> 8;
				buf[3] = rtpseq;
			}
			if (sendto(outfd, buf, bufbase + bufsize, 0, outaddrs->ai_addr, outaddrs->ai_addrlen) < 0) {
				if (errno != EINTR) {
					fprintf(stderr, "Socket send failure: %m\n");
					return 0;
				}
			}
			rtpseq++;
			bufsize = 0;
		}
	}

	if (bufsize) {
		if (usertp) {
			buf[2] = rtpseq >> 8;
			buf[3] = rtpseq;
		}
		if (sendto(outfd, buf, bufbase + bufsize, 0, outaddrs->ai_addr, outaddrs->ai_addrlen) < 0) {
			if (errno != EINTR)
				fprintf(stderr, "Socket send failure: %m\n");
		}
	}

	return 0;
}

static int gnutv_data_create_decoder_filter(int adapter, int demux, uint16_t pid, int pestype)
{
	int demux_fd = -1;

	// open the demuxer
	if ((demux_fd = dvbdemux_open_demux(adapter, demux, 0)) < 0) {
		return -1;
	}

	// create a section filter
	if (dvbdemux_set_pes_filter(demux_fd, pid, DVBDEMUX_INPUT_FRONTEND, DVBDEMUX_OUTPUT_DECODER, pestype, 1)) {
		close(demux_fd);
		return -1;
	}

	// done
	return demux_fd;
}

static int gnutv_data_create_dvr_filter(int adapter, int demux, uint16_t pid)
{
	int demux_fd = -1;

	// open the demuxer
	if ((demux_fd = dvbdemux_open_demux(adapter, demux, 0)) < 0) {
		return -1;
	}

	// create a section filter
	if (dvbdemux_set_pid_filter(demux_fd, pid, DVBDEMUX_INPUT_FRONTEND, DVBDEMUX_OUTPUT_DVR, 1)) {
		close(demux_fd);
		return -1;
	}

	// done
	return demux_fd;
}

static void gnutv_data_decoder_pmt(struct mpeg_pmt_section *pmt)
{
	int audio_pid = -1;
	int video_pid = -1;
	struct mpeg_pmt_stream *cur_stream;
	mpeg_pmt_section_streams_for_each(pmt, cur_stream) {
		switch(cur_stream->stream_type) {
		case 1:
		case 2: // video
			video_pid = cur_stream->pid;
			break;

		case 3:
		case 4: // audio
			audio_pid = cur_stream->pid;
			break;
		}
	}

	if (audio_pid != -1) {
		int fd = gnutv_data_create_decoder_filter(adapter_id, demux_id, audio_pid, DVBDEMUX_PESTYPE_AUDIO);
		if (fd < 0) {
			fprintf(stderr, "Unable to create dvr filter for PID %i\n", audio_pid);
		} else {
			gnutv_data_append_pid_fd(audio_pid, fd);
		}
	}
	if (video_pid != -1) {
		int fd = gnutv_data_create_decoder_filter(adapter_id, demux_id, video_pid, DVBDEMUX_PESTYPE_VIDEO);
		if (fd < 0) {
			fprintf(stderr, "Unable to create dvr filter for PID %i\n", video_pid);
		} else {
			gnutv_data_append_pid_fd(video_pid, fd);
		}
	}
	int fd = gnutv_data_create_decoder_filter(adapter_id, demux_id, pmt->pcr_pid, DVBDEMUX_PESTYPE_PCR);
	if (fd < 0) {
		fprintf(stderr, "Unable to create dvr filter for PID %i\n", pmt->pcr_pid);
	} else {
		gnutv_data_append_pid_fd(pmt->pcr_pid, fd);
	}
}

static void gnutv_data_dvr_pmt(struct mpeg_pmt_section *pmt)
{
	struct mpeg_pmt_stream *cur_stream;
	mpeg_pmt_section_streams_for_each(pmt, cur_stream) {
		int fd = gnutv_data_create_dvr_filter(adapter_id, demux_id, cur_stream->pid);
		if (fd < 0) {
			fprintf(stderr, "Unable to create dvr filter for PID %i\n", cur_stream->pid);
		} else {
			gnutv_data_append_pid_fd(cur_stream->pid, fd);
		}
	}
}

static void gnutv_data_append_pid_fd(int pid, int fd)
{
	struct pid_fd *tmp;
	if ((tmp = realloc(pid_fds, (pid_fds_count +1) * sizeof(struct pid_fd))) == NULL) {
		fprintf(stderr, "Out of memory when adding a new pid_fd\n");
		exit(1);
	}
	tmp[pid_fds_count].pid = pid;
	tmp[pid_fds_count].fd = fd;
	pid_fds_count++;
	pid_fds = tmp;
}

static void gnutv_data_free_pid_fds()
{
	if (pid_fds_count) {
		int i;
		for(i=0; i< pid_fds_count; i++) {
			close(pid_fds[i].fd);
		}
	}
	if (pid_fds)
		free(pid_fds);

	pid_fds_count = 0;
	pid_fds = NULL;
}
