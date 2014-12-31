/*
 * lsdvb - list PCI DVB devices
 *
 * Copyright (C) 2010 Manu Abraham <abraham.manu@gmail.com>
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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <dirent.h>

#include <linux/dvb/frontend.h>

#define DVB_DIR			"/dev/dvb"
#define DVB_DEV_DIR		"/dev/dvb"
#define DVB_SYSFS_DIR		"/sys/class/dvb"

/**
 * DVB sysfs entries are found thus:
 * /sys/class/dvb/dvb0.frontend0/device/dvb/dvb0.frontend0/uevent for DVB_ADAPTER_NUM, DVB_DEVICE_NUM
 *
 * /sys/class/dvb/dvb0.frontend0/device/device for PCI_DEVICE_ID
 * /sys/class/dvb/dvb0.frontend0/device/vendor for PCI_VENDOR_ID
 * /sys/class/dvb/dvb0.frontend0/device/irq    for PCI_IRQ
 */

#define MAXLEN		64

int get_frontend_entry(DIR *dvb_dir, char *frontend_dev)
{
	struct dirent *adap_dir;
	char tmp[MAXLEN];

	adap_dir = readdir(dvb_dir);
	if (adap_dir != NULL) {
		/**
		 * Directory structure follows:
		 * .
		 * ..
		 * ~dvb0.demux0
		 * ~dvb0.dvr0
		 * ~dvb0.frontend0
		 * ~dvb0.net0
		 * ~dvb1.demux0
		 * ~dvb1.dvr0
		 * ~dvb1.frontend0
		 * ~dvb1.net0
		 */

		/* print information */
		sprintf(tmp, "%s/%s", DVB_SYSFS_DIR, adap_dir->d_name);
		/* search for a dir entry with string "frontend" */
		if (strstr(tmp, "frontend")) {
			/* found a dvbX.frontendY */
			strcpy(frontend_dev, tmp);
			return 0;
		} else {
			return -2;
		}
	} else {
		return -1;
	}
}


/* read device number and adapter number from the uevent file */
int read_frontend_uevent(char *uevent, int *device, int *adapter)
{
	char uf_name[MAXLEN];
	FILE *ufile;
	char line[128];
	char *token = NULL;

	/* get uevent file */
	sprintf(uf_name, "%s/%s", uevent, "uevent");
	ufile = fopen(uf_name, "r");
	if (ufile == NULL) {
		fprintf(stderr, "File: %s open failed\n", uf_name);
		return -1;
	}

	while (1) {
		if (fgets(line, sizeof (line), ufile) == NULL)
			return -1;

		if (strstr(line, "DVB_ADAPTER_NUM")) {
			token = strtok(line, "=");
			token = strtok(NULL, "=");
			*adapter = atoi(token);
		}

		if (strstr(line, "DVB_DEVICE_NUM")) {
			token = strtok(line, "=");
			token = strtok(NULL, "=");
			*device = atoi(token);
		}
	}

	fclose(ufile);
	return 0;
}

int read_device_uevent(char *uevent,
		       char *drv,
		       int *pci_ven,
		       int *pci_dev,
		       int *pci_subven,
		       int *pci_subdev,
		       int *domain,
		       int *bus,
		       int *device,
		       int *fn)
{
	char uf_name[MAXLEN];
	FILE *ufile;
	char line[128];
	char *token = NULL;
	char *tmp;
	unsigned int i;

	/* get uevent file */
	sprintf(uf_name, "%s/%s", uevent, "device/uevent");
	ufile = fopen(uf_name, "r");
	if (ufile == NULL) {
		fprintf(stderr, "File: %s open failed\n", uf_name);
		return -1;
	}

	while (1) {
		if (fgets(line, sizeof (line), ufile) == NULL)
			return -1;

		if (strstr(line, "DRIVER")) {
			token = strtok(line, "=");
			token = strtok(NULL, "=");
			for (i = 0; i < strlen(token); i++) {
				if (token[i] == '\n' || token[i] == '\r')
					token[i] = '\0';
			}
			strcpy(drv, token);
		}
		if (strstr(line, "PCI_ID")) {
			token = strtok(line, "=");
			token = strtok(NULL, "=");
			*pci_ven = atoi(strtok(token, ":"));
			*pci_dev = atoi(strtok(NULL, ":"));
		}
		if (strstr(line, "PCI_SUBSYS_ID")) {
			token = strtok(line, "=");
			token = strtok(NULL, "=");
			*pci_subven = atoi(strtok(token, ":"));
			*pci_subdev = atoi(strtok(NULL, ":"));

		}
		if (strstr(line, "PCI_SLOT_NAME")) {
			token = strtok(line, "=");
			token = strtok(NULL, "=");
			*domain = atoi(strtok(token, ":"));
			*bus = atoi(strtok(NULL, ":"));
			tmp = strtok(NULL, ":");
			*device = atoi(strtok(tmp, "."));
			*fn = atoi(strtok(NULL, "."));
		}
	}

	fclose(ufile);
	return 0;
}

/* retrieve frontend info */
int get_frontend_info(int adapter, char *frontend)
{
	DIR *dev_dir;
	struct dirent *adap_dir;
	char tmp[MAXLEN];

	sprintf(tmp, "%s/adapter%d", DVB_DIR, adapter);
	dev_dir = opendir(tmp);
	if (!dev_dir) {
		fprintf(stderr, "ERROR: Opening %s directory\n", tmp);
		return - 1;
	}

	while ((adap_dir = readdir(dev_dir)) != NULL) {
		/**
		 * Directory structure follows:
		 * .
		 * ..
		 * frontend0
		 * frontend1
		 * demux0
		 * dvr0
		 * net0
		 */
		if (!strcmp(adap_dir->d_name, "."))
			continue;
		if (!strcmp(adap_dir->d_name, ".."))
			continue;

		/* search for a dir entry with string "frontend" */
		if (strstr(adap_dir->d_name, "frontend")) {
			/* found a dvbX.frontendY */
			strcpy(frontend, adap_dir->d_name);
		}

	}

	closedir(dev_dir);
	return 0;
}

int main(void)
{
	DIR *sys_dir;
	char frontend_dev[MAXLEN];
	int entry, device, adapter;
	char drv[MAXLEN];
	int pci_ven, pci_dev, pci_subven, pci_subdev;
	int domain, bus, dev, fn;
	int dev_prev = -1;

	char tmp[MAXLEN];
	char frontend[MAXLEN];
	char fedev[MAXLEN];
	int fd, ret;
	struct dvb_frontend_info info;

	static char *fe_type[] = {
		[0] = "FE_QPSK",
		[1] = "FE_QAM",
		[2] = "FE_OFDM",
		[3] = "FE_ATSC",
	};

	fprintf(stderr, "\n\t\tlsdvb: Simple utility to list PCI/PCIe DVB devices\n");
	fprintf(stderr, "\t\tVersion: 0.0.4\n");
	fprintf(stderr, "\t\tCopyright (C) Manu Abraham\n");
	sys_dir = opendir(DVB_SYSFS_DIR);
	if (!sys_dir) {
		fprintf(stderr, "ERROR: Opening %s directory\n", DVB_SYSFS_DIR);
		return - 1;
	}

	while (1) {
		entry = get_frontend_entry(sys_dir, frontend_dev);
		if (entry == 0) {

			read_device_uevent(frontend_dev,
					   drv,
					   &pci_ven,
					   &pci_dev,
					   &pci_subven,
					   &pci_subdev,
					   &domain,
					   &bus,
					   &dev,
					   &fn);

			read_frontend_uevent(frontend_dev, &device, &adapter);
			if (device != dev_prev) {
				fprintf(stderr, "\n%s (%d:%d %d:%d) on PCI Domain:%d Bus:%d Device:%d Function:%d\n",
					drv, pci_ven, pci_dev, pci_subven, pci_subdev, domain, bus, dev, fn);
			}

			fprintf(stderr, "\tDEVICE:%d ADAPTER:%d ", device, adapter);
			dev_prev = device;

			get_frontend_info(adapter, frontend);
			sprintf(fedev, "%s/adapter%d/%s", DVB_DIR, adapter, frontend);
			fd = open(fedev, O_RDWR | O_NONBLOCK);
			if (fd < 0) {
				fprintf(stderr, "ERROR: Open %s failed\n", frontend);
				return -1;
			}

			ret = ioctl(fd, FE_GET_INFO, &info);
			if (ret < 0) {
				fprintf(stderr, "ERROR: IOCTL failed\n");
				return -1;
			}
			strncpy(tmp, frontend + 8, 1);
			fprintf(stderr, "FRONTEND:%d (%s) \n\t\t %s Fmin=%dMHz Fmax=%dMHz\n",
				atoi(tmp),
				info.name,
				fe_type[info.type],
				info.type == 0 ? info.frequency_min / 1000: info.frequency_min / 1000000,
				info.type == 0 ? info.frequency_max / 1000: info.frequency_max / 1000000);
		}

		if (entry == -1)
			break;
		if (entry == -2)
			continue;

	}

	closedir(sys_dir);

	return 0;
}
