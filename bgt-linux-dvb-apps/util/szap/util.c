/*
 * util functions for various ?zap implementations
 *
 * Copyright (C) 2001 Johannes Stezenbach (js@convergence.de)
 * for convergence integrated media
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>


int set_pesfilter(int dmxfd, int pid, int pes_type, int dvr)
{
	struct dmx_pes_filter_params pesfilter;

	/* ignore this pid to allow radio services */
	if (pid < 0 || pid >= 0x1fff || (pid == 0 && pes_type != DMX_PES_OTHER))
		return 0;

	if (dvr) {
		int buffersize = 64 * 1024;
		if (ioctl(dmxfd, DMX_SET_BUFFER_SIZE, buffersize) == -1)
			perror("DMX_SET_BUFFER_SIZE failed");
	}

	pesfilter.pid = pid;
	pesfilter.input = DMX_IN_FRONTEND;
	pesfilter.output = dvr ? DMX_OUT_TS_TAP : DMX_OUT_DECODER;
	pesfilter.pes_type = pes_type;
	pesfilter.flags = DMX_IMMEDIATE_START;

	if (ioctl(dmxfd, DMX_SET_PES_FILTER, &pesfilter) == -1) {
		fprintf(stderr, "DMX_SET_PES_FILTER failed "
			"(PID = 0x%04x): %d %m\n", pid, errno);
		return -1;
	}
	return 0;
}


int get_pmt_pid(char *dmxdev, int sid)
{
	int patfd, count;
	int pmt_pid = 0;
	int patread = 0;
	int section_length;
	unsigned char buft[4096];
	unsigned char *buf = buft;
	struct dmx_sct_filter_params f;

	memset(&f, 0, sizeof(f));
	f.pid = 0;
	f.filter.filter[0] = 0x00;
	f.filter.mask[0] = 0xff;
	f.timeout = 0;
	f.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

	if ((patfd = open(dmxdev, O_RDWR)) < 0) {
		perror("openening pat demux failed");
		return -1;
	}

	if (ioctl(patfd, DMX_SET_FILTER, &f) == -1) {
		perror("ioctl DMX_SET_FILTER failed");
		close(patfd);
		return -1;
	}

	while (!patread) {
		if (((count = read(patfd, buf, sizeof(buft))) < 0) && errno == EOVERFLOW)
			count = read(patfd, buf, sizeof(buft));

		if (count < 0) {
			perror("read_sections: read error");
			close(patfd);
			return -1;
		}
		section_length = ((buf[1] & 0x0f) << 8) | buf[2];
		if (count != section_length + 3)
			continue;

		buf += 8;
		section_length -= 8;

		patread = 1; /* assumes one section contains the whole pat */
		while (section_length > 0) {
			int service_id = (buf[0] << 8) | buf[1];

			if (service_id == sid) {
				pmt_pid = ((buf[2] & 0x1f) << 8) | buf[3];
				section_length = 0;
			}
			buf += 4;
			section_length -= 4;
		}
	}
	close(patfd);
	return pmt_pid;
}

char *type_str[] = {
	"QPSK",
	"QAM",
	"OFDM",
	"ATSC",
};

/* to be used with v3 drivers */
int check_frontend_v3(int fd, enum fe_type type)
{
	struct dvb_frontend_info info;
	int ret;

	ret = ioctl(fd, FE_GET_INFO, &info);
	if (ret < 0) {
		perror("ioctl FE_GET_INFO failed");
		close(fd);
		ret = -1;
		goto exit;
	}
	if (info.type != type) {
		fprintf(stderr, "Not a valid %s device!\n", type_str[type]);
		close(fd);
		ret = -EINVAL;
		goto exit;
	}
exit:
	return ret;
}

char *del_str[] = {
	"UNDEFINED",
	"DVB-C (A)",
	"DVB-C (B)",
	"DVB-T",
	"DSS",
	"DVB-S",
	"DVB-S2",
	"DVB-H",
	"ISDB-T",
	"ISDB-S",
	"ISDB-C",
	"ATSC",
	"ATSC-M/H",
	"DTMB",
	"CMMB",
	"DAB",
	"DVB-T2",
	"TURBO",
	"QAM (C)",
};

static int map_delivery_mode(fe_type_t *type, enum fe_delivery_system delsys)
{
	switch (delsys) {
	case SYS_DSS:
	case SYS_DVBS:
	case SYS_DVBS2:
	case SYS_TURBO:
		*type = FE_QPSK;
		break;
	case SYS_DVBT:
	case SYS_DVBT2:
	case SYS_DVBH:
	case SYS_ISDBT:
		*type = FE_OFDM;
		break;
	case SYS_DVBC_ANNEX_A:
	case SYS_DVBC_ANNEX_C:
		*type = FE_QAM;
		break;
	case SYS_ATSC:
	case SYS_DVBC_ANNEX_B:
		*type = FE_ATSC;
		break;
	default:
		fprintf(stderr, "Delivery system unsupported, please report to linux-media ML\n");
		return -1;
	}
	return 0;
}

int get_property(int fd, uint32_t pcmd, uint32_t *len, uint8_t *data)
{
	struct dtv_property p, *b;
	struct dtv_properties cmd;
	int ret;

	p.cmd = pcmd;
	cmd.num = 1;
	cmd.props = &p;
	b = &p;

	ret = ioctl(fd, FE_GET_PROPERTY, &cmd);
	if (ret < 0) {
		fprintf(stderr, "FE_SET_PROPERTY returned %d\n", ret);
		return -1;
	}
	memcpy(len, &b->u.buffer.len, sizeof (uint32_t));
	memcpy(data, b->u.buffer.data, *len);
	return 0;
}

int set_property(int fd, uint32_t cmd, uint32_t data)
{
	struct dtv_property p, *b;
	struct dtv_properties c;
	int ret;

	p.cmd = cmd;
	c.num = 1;
	c.props = &p;
	b = &p;
	b->u.data = data;
	ret = ioctl(fd, FE_SET_PROPERTY, &c);
	if (ret < 0) {
		fprintf(stderr, "FE_SET_PROPERTY returned %d\n", ret);
		return -1;
	}
	return 0;
}

int dvbfe_get_delsys(int fd, fe_delivery_system_t *delsys)
{
	uint32_t len;
	/* Buggy API design */
	return get_property(fd, DTV_DELIVERY_SYSTEM, &len, (uint8_t *)delsys);
}

int dvbfe_set_delsys(int fd, enum fe_delivery_system delsys)
{
	return set_property(fd, DTV_DELIVERY_SYSTEM, delsys);
}

int dvbfe_enum_delsys(int fd, uint32_t *len, uint8_t *data)
{
	return get_property(fd, DTV_ENUM_DELSYS, len, data);
}

int dvbfe_get_version(int fd, int *major, int *minor)
{
	struct dtv_property p, *b;
	struct dtv_properties cmd;
	int ret;

	p.cmd = DTV_API_VERSION;
	cmd.num = 1;
	cmd.props = &p;
	b = &p;

	ret = ioctl(fd, FE_GET_PROPERTY, &cmd);
	if (ret < 0) {
		fprintf(stderr, "FE_GET_PROPERTY failed, ret=%d\n", ret);
		return -1;
	}
	*major = (b->u.data >> 8) & 0xff;
	*minor = b->u.data & 0xff;
	return 0;
}

int check_frontend_multi(int fd, enum fe_type type, uint32_t *mstd)
{
	int ret;

	enum fe_type delmode;
	unsigned int i, valid_delsys = 0;
	uint32_t len;
	uint8_t data[32];

	ret = dvbfe_enum_delsys(fd, &len, data);
	if (ret) {
		fprintf(stderr, "enum_delsys failed, ret=%d\n", ret);
		ret = -EIO;
		goto exit;
	}
	fprintf(stderr, "\t FE_CAN { ");
	for (i = 0; i < len; i++) {
		if (i < len - 1)
			fprintf(stderr, "%s + ", del_str[data[i]]);
		else
			fprintf(stderr, "%s", del_str[data[i]]);
	}
	fprintf(stderr, " }\n");
	/* check whether frontend can support our delivery */
	for (i = 0; i < len; i++) {
		map_delivery_mode(&delmode, data[i]);
		if (type == delmode) {
			valid_delsys = 1;
			ret = 0;
			break;
		}
	}
	if (!valid_delsys) {
		fprintf(stderr, "Not a valid %s device!\n", type_str[type]);
		ret = -EINVAL;
		goto exit;
	}
	*mstd = len; /* mstd has supported delsys count */
exit:
	return ret;
}

int check_frontend(int fd, enum fe_type type, uint32_t *mstd)
{
	int major, minor, ret;

	ret = dvbfe_get_version(fd, &major, &minor);
	if (ret)
		goto exit;
	fprintf(stderr, "Version: %d.%d  ", major, minor);
	if ((major == 5) && (minor > 8)) {
		ret = check_frontend_multi(fd, type, mstd);
		if (ret)
			goto exit;
	} else {
		ret = check_frontend_v3(fd, type);
		if (ret)
			goto exit;
	}
exit:
	return ret;
}
