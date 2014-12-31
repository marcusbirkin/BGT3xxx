/**
 * dvbsec_cfg (i.e. linuxtv sec format) configuration file support.
 *
 * Copyright (c) 2005 by Andrew de Quincey <adq_dvb@lidskialf.net>
 *
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <linux/types.h>
#include "dvbsec_cfg.h"

int dvbcfg_issection(char* line, char* sectionname)
{
	int len;

	len = strlen(line);
	if (len < 2)
		return 0;

	if ((line[0] != '[') || (line[len-1] != ']'))
		return 0;

	line++;
	while(isspace(*line))
		line++;

	if (strncmp(line, sectionname, strlen(sectionname)))
		return 0;

	return 1;
}

char* dvbcfg_iskey(char* line, char* keyname)
{
	int len = strlen(keyname);

	/* does the key match? */
	if (strncmp(line, keyname, len))
		return NULL;

	/* skip keyname & any whitespace */
	line += len;
	while(isspace(*line))
		line++;

	/* should be the '=' sign */
	if (*line != '=')
		return 0;

	/* more whitespace skipping */
	line++;
	while(isspace(*line))
		line++;

	/* finally, return the value */
	return line;
}

int dvbsec_cfg_load(FILE *f,
		    void *arg,
		    dvbsec_cfg_callback cb)
{
	struct dvbsec_config tmpsec;
	char *linebuf = NULL;
	size_t line_size = 0;
	int len;
	int insection = 0;
	char *value;

	/* process each line */
	while((len = getline(&linebuf, &line_size, f)) > 0) {
		char *line = linebuf;

		/* chop any comments */
		char *hashpos = strchr(line, '#');
		if (hashpos)
			*hashpos = 0;
		char *lineend = line + strlen(line);

		/* trim the line */
		while(*line && isspace(*line))
			line++;
		while((lineend != line) && isspace(*(lineend-1)))
			lineend--;
		*lineend = 0;

		/* skip blank lines */
		if (*line == 0)
			continue;

		if (dvbcfg_issection(line, "sec")) {
			if (insection) {
				if (cb(arg, &tmpsec))
					return 0;
			}
			insection = 1;
			memset(&tmpsec, 0, sizeof(tmpsec));

		} else if ((value = dvbcfg_iskey(line, "name")) != NULL) {
			strncpy(tmpsec.id, value, sizeof(tmpsec.id));
		} else if ((value = dvbcfg_iskey(line, "switch-frequency")) != NULL) {
			tmpsec.switch_frequency = atoi(value);
		} else if ((value = dvbcfg_iskey(line, "lof-lo-v")) != NULL) {
			tmpsec.lof_lo_v = atoi(value);
		} else if ((value = dvbcfg_iskey(line, "lof-lo-h")) != NULL) {
			tmpsec.lof_lo_h = atoi(value);
		} else if ((value = dvbcfg_iskey(line, "lof-lo-l")) != NULL) {
			tmpsec.lof_lo_l = atoi(value);
		} else if ((value = dvbcfg_iskey(line, "lof-lo-r")) != NULL) {
			tmpsec.lof_lo_r = atoi(value);
		} else if ((value = dvbcfg_iskey(line, "lof-hi-v")) != NULL) {
			tmpsec.lof_hi_v = atoi(value);
		} else if ((value = dvbcfg_iskey(line, "lof-hi-h")) != NULL) {
			tmpsec.lof_hi_h = atoi(value);
		} else if ((value = dvbcfg_iskey(line, "lof-hi-l")) != NULL) {
			tmpsec.lof_hi_l = atoi(value);
		} else if ((value = dvbcfg_iskey(line, "lof-hi-r")) != NULL) {
			tmpsec.lof_hi_r = atoi(value);
		} else if ((value = dvbcfg_iskey(line, "config-type")) != NULL) {
			if (!strcasecmp(value, "none")) {
				tmpsec.config_type = DVBSEC_CONFIG_NONE;
			} else if (!strcasecmp(value, "power")) {
				tmpsec.config_type = DVBSEC_CONFIG_POWER;
			} else if (!strcasecmp(value, "standard")) {
				tmpsec.config_type = DVBSEC_CONFIG_STANDARD;
			} else if (!strcasecmp(value, "advanced")) {
				tmpsec.config_type = DVBSEC_CONFIG_ADVANCED;
			} else {
				insection = 0;
			}
		} else if ((value = dvbcfg_iskey(line, "cmd-lo-v")) != NULL) {
			strncpy(tmpsec.adv_cmd_lo_v, value, sizeof(tmpsec.adv_cmd_lo_v));
		} else if ((value = dvbcfg_iskey(line, "cmd-lo-h")) != NULL) {
			strncpy(tmpsec.adv_cmd_lo_h, value, sizeof(tmpsec.adv_cmd_lo_h));
		} else if ((value = dvbcfg_iskey(line, "cmd-lo-r")) != NULL) {
			strncpy(tmpsec.adv_cmd_lo_r, value, sizeof(tmpsec.adv_cmd_lo_r));
		} else if ((value = dvbcfg_iskey(line, "cmd-lo-l")) != NULL) {
			strncpy(tmpsec.adv_cmd_lo_l, value, sizeof(tmpsec.adv_cmd_lo_l));
		} else if ((value = dvbcfg_iskey(line, "cmd-hi-v")) != NULL) {
			strncpy(tmpsec.adv_cmd_hi_v, value, sizeof(tmpsec.adv_cmd_hi_v));
		} else if ((value = dvbcfg_iskey(line, "cmd-hi-h")) != NULL) {
			strncpy(tmpsec.adv_cmd_hi_h, value, sizeof(tmpsec.adv_cmd_hi_h));
		} else if ((value = dvbcfg_iskey(line, "cmd-hi-r")) != NULL) {
			strncpy(tmpsec.adv_cmd_hi_r, value, sizeof(tmpsec.adv_cmd_hi_r));
		} else if ((value = dvbcfg_iskey(line, "cmd-hi-l")) != NULL) {
			strncpy(tmpsec.adv_cmd_hi_l, value, sizeof(tmpsec.adv_cmd_hi_l));
		} else {
			insection = 0;
		}
	}

	// output the final section if there is one
	if (insection) {
		if (cb(arg, &tmpsec))
			return 0;
	}

	if (linebuf)
		free(linebuf);
	return 0;
}

static int dvbsec_cfg_find_callback(void *arg, struct dvbsec_config *sec);
static int dvbsec_cfg_find_default(const char *sec_id, struct dvbsec_config *sec);

struct findparams {
	const char *sec_id;
	struct dvbsec_config *sec_dest;
};

int dvbsec_cfg_find(const char *config_file,
		    const char *sec_id,
		    struct dvbsec_config *sec)
{
	struct findparams findp;

	// clear the structure
	memset(sec, 0, sizeof(struct dvbsec_config));

	// open the file
	if (config_file != NULL) {
		FILE *f = fopen(config_file, "r");
		if (f == NULL)
			return -EIO;

		// parse each entry
		findp.sec_id = sec_id;
		findp.sec_dest = sec;
		dvbsec_cfg_load(f, &findp, dvbsec_cfg_find_callback);

		// done
		fclose(f);

		// find it?
		if (sec->id[0])
			return 0;
	}

	return dvbsec_cfg_find_default(sec_id, sec);
}

static int dvbsec_cfg_find_callback(void *arg, struct dvbsec_config *sec)
{
	struct findparams *findp = arg;

	if (strcmp(findp->sec_id, sec->id))
		return 0;

	memcpy(findp->sec_dest, sec, sizeof(struct dvbsec_config));
	return 1;
}

int dvbsec_cfg_save(FILE *f,
		    struct dvbsec_config *secs,
		    int count)
{
	int i;

	for(i=0; i<count; i++) {
		char *config_type = "";
		switch(secs[i].config_type) {
		case DVBSEC_CONFIG_NONE:
			config_type = "none";
			break;
		case DVBSEC_CONFIG_POWER:
			config_type = "power";
			break;
		case DVBSEC_CONFIG_STANDARD:
			config_type = "standard";
			break;
		case DVBSEC_CONFIG_ADVANCED:
			config_type = "advanced";
			break;
		}

		fprintf(f, "[lnb]\n");
		fprintf(f, "switch-frequency=%i\n", secs[i].switch_frequency);
		if (secs[i].lof_lo_v)
			fprintf(f, "lof-lo-v=%i\n", secs[i].lof_lo_v);
		if (secs[i].lof_lo_h)
			fprintf(f, "lof-lo-h=%i\n", secs[i].lof_lo_h);
		if (secs[i].lof_lo_l)
			fprintf(f, "lof-lo-l=%i\n", secs[i].lof_lo_l);
		if (secs[i].lof_lo_r)
			fprintf(f, "lof-lo-r=%i\n", secs[i].lof_lo_r);
		if (secs[i].lof_hi_v)
			fprintf(f, "lof-hi-v=%i\n", secs[i].lof_hi_v);
		if (secs[i].lof_hi_h)
			fprintf(f, "lof-hi-h=%i\n", secs[i].lof_hi_h);
		if (secs[i].lof_hi_l)
			fprintf(f, "lof-hi-l=%i\n", secs[i].lof_hi_l);
		if (secs[i].lof_hi_r)
			fprintf(f, "lof-hi-r=%i\n", secs[i].lof_hi_r);
		fprintf(f, "config-type=%s\n", config_type);

		if (secs[i].config_type == DVBSEC_CONFIG_ADVANCED) {
			if (secs[i].adv_cmd_lo_h[0])
				fprintf(f, "cmd-lo-h=%s\n", secs[i].adv_cmd_lo_h);
			if (secs[i].adv_cmd_lo_v[0])
				fprintf(f, "cmd-lo-v=%s\n", secs[i].adv_cmd_lo_v);
			if (secs[i].adv_cmd_lo_r[0])
				fprintf(f, "cmd-lo-r=%s\n", secs[i].adv_cmd_lo_r);
			if (secs[i].adv_cmd_lo_l[0])
				fprintf(f, "cmd-lo-l=%s\n", secs[i].adv_cmd_lo_l);
			if (secs[i].adv_cmd_hi_h[0])
				fprintf(f, "cmd-hi-h=%s\n", secs[i].adv_cmd_hi_h);
			if (secs[i].adv_cmd_hi_v[0])
				fprintf(f, "cmd-hi-v=%s\n", secs[i].adv_cmd_hi_v);
			if (secs[i].adv_cmd_hi_r[0])
				fprintf(f, "cmd-hi-r=%s\n", secs[i].adv_cmd_hi_r);
			if (secs[i].adv_cmd_hi_l[0])
				fprintf(f, "cmd-hi-l=%s\n", secs[i].adv_cmd_hi_l);
		}

		fprintf(f, "\n");
	}

	return 0;
}

static struct dvbsec_config defaults[] = {

	{
		.id = "NULL",
		.config_type = DVBSEC_CONFIG_STANDARD,
	},
	{
		.id = "UNIVERSAL",
		.switch_frequency = 11700000,
		.lof_lo_v = 9750000,
		.lof_lo_h = 9750000,
		.lof_hi_v = 10600000,
		.lof_hi_h = 10600000,
		.config_type = DVBSEC_CONFIG_STANDARD,
	},
	{
		.id = "DBS",
		.switch_frequency = 0,
		.lof_lo_v = 11250000,
		.lof_lo_h = 11250000,
		.config_type = DVBSEC_CONFIG_STANDARD,
	},
	{
		.id = "STANDARD",
		.switch_frequency = 0,
		.lof_lo_v = 10000000,
		.lof_lo_h = 10000000,
		.config_type = DVBSEC_CONFIG_STANDARD,
	},
	{
		.id = "ENHANCED",
		.switch_frequency = 0,
		.lof_lo_v = 9750000,
		.lof_lo_h = 9750000,
		.config_type = DVBSEC_CONFIG_STANDARD,
	},
	{
		.id = "C-BAND",
		.switch_frequency = 0,
		.lof_lo_v = 5150000,
		.lof_lo_h = 5150000,
		.config_type = DVBSEC_CONFIG_POWER,
	},
	{
		.id = "C-MULTI",
		.switch_frequency = 0,
		.lof_lo_v = 5150000,
		.lof_lo_h = 5750000,
		.config_type = DVBSEC_CONFIG_POWER,
	},
};
#define defaults_count (sizeof(defaults) / sizeof(struct dvbsec_config))

static int dvbsec_cfg_find_default(const char *sec_id,
				   struct dvbsec_config *sec)
{
	unsigned int i;

	for(i=0; i< defaults_count; i++) {
		if (!strncmp(sec_id, defaults[i].id, sizeof(defaults[i].id))) {
			memcpy(sec, &defaults[i], sizeof(struct dvbsec_config));
			return 0;
		}
	}

	return -1;
}
