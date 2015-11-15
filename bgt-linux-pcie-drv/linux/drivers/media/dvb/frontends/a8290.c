/*
	Allegro A8290 LNB controller

	Copyright (C) Manu Abraham <abraham.manu@gmail.com>

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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mutex.h>

#include <linux/dvb/frontend.h>
#include "dvb_frontend.h"

#include "a8290.h"

static unsigned int verbose;
module_param(verbose, int, 0644);
static unsigned int cable_comp;
module_param(cable_comp, int, 0644);

#define FE_ERROR				0
#define FE_NOTICE				1
#define FE_INFO					2
#define FE_DEBUG				3
#define FE_DEBUGREG				4

#define dprintk(__y, __z, format, arg...) do {						\
	if (__z) {									\
		if	((verbose > FE_ERROR) && (verbose > __y))			\
			printk(KERN_ERR "%s: " format "\n", __func__ , ##arg);		\
		else if	((verbose > FE_NOTICE) && (verbose > __y))			\
			printk(KERN_NOTICE "%s: " format "\n", __func__ , ##arg);	\
		else if ((verbose > FE_INFO) && (verbose > __y))			\
			printk(KERN_INFO "%s: " format "\n", __func__ , ##arg);		\
		else if ((verbose > FE_DEBUG) && (verbose > __y))			\
			printk(KERN_DEBUG "%s: " format "\n", __func__ , ##arg);	\
	} else {									\
		if (verbose > __y)							\
			printk(format, ##arg);						\
	}										\
} while (0)


struct a8290_state {
	u32	*verbose;
	const struct a8290_config	*config;
	struct i2c_adapter		*i2c;
	struct dvb_frontend		*frontend;
	u8	sleep;
};

static int a8290_i2c(struct a8290_state *a8290, u8 *val, int len, bool rd)
{
	int ret;
	struct i2c_msg msg[1] = {
		{ .addr = a8290->config->address, .len = len, .buf = val, }
	};
	if (rd)
		msg[0].flags = I2C_M_RD;
	else
		msg[0].flags = 0;

	ret = i2c_transfer(a8290->i2c, msg, 1);
	if (ret == 1) {
		ret = 0;
	} else {
		dprintk(FE_ERROR, 1, "I/O error=%d rd=%d", ret, rd);
		ret = -EREMOTEIO;
	}
	return ret;
}

static int a8290_wr(struct a8290_state *a8290, u8 *val, int len)
{
	return a8290_i2c(a8290, val, len, 0);
}
static int a8290_rd(struct a8290_state *a8290, u8 *val, int len)
{
	return a8290_i2c(a8290, val, len, 1);
}


static int a8290_init(struct a8290_state *a8290)
{
	const struct a8290_config *config = a8290->config;
	int ret;
	u8 data;

	ret = a8290_rd(a8290, &data, 1);
	if (ret)
		goto err;
	ret = a8290_rd(a8290, &data, 1);
	if (ret)
		goto err;
	if (data & 0xc4) {
		dprintk(FE_ERROR, 1, "Device in unknown state!");
		goto err;
	}
	data = 0x82;
	if (config->ext_mod)
		data |= 0x01;
	if (config->i2c_txrx_ctl)
		data |= 0x20;
	ret = a8290_wr(a8290, &data, 1);
	if (ret)
		goto err;
	data = 0x31;
	ret = a8290_wr(a8290, &data, 1);
	if (ret)
		goto err;
	msleep(500);
	a8290->sleep = 0;
	return ret;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
static int a8290_set_voltage(struct dvb_frontend* fe, enum fe_sec_voltage voltage)
#else
static int a8290_set_voltage(struct dvb_frontend* fe, fe_sec_voltage_t voltage)
#endif
{
	struct a8290_state *a8290 = fe->sec_priv;
	u8 data;
	int ret;

	if (a8290->sleep) {
		ret = a8290_init(a8290);
		if (ret)
			goto err;
	}
	data = 0x30;
	switch (voltage) {
	case SEC_VOLTAGE_13:
		if (cable_comp)
			data |= 0x04; /* 14.04V */
		else
			data |= 0x01; /* 13.04V */
		break;
	case SEC_VOLTAGE_18:
		if (cable_comp)
			data |= 0x0B; /* 19.04V */
		else
			data |= 0x08; /* 18.04V */
		break;
	case SEC_VOLTAGE_OFF:
		/* LNB power off! */
		break;
	default:
		return -EINVAL;
	};
	ret = a8290_wr(a8290, &data, 1);
	if (ret)
		goto err;
	msleep(20);
	return ret;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
static int a8290_set_tone(struct dvb_frontend *fe, enum fe_sec_tone_mode tone)
#else
static int a8290_set_tone(struct dvb_frontend *fe, fe_sec_tone_mode_t tone)
#endif
{
	struct a8290_state *a8290 = fe->sec_priv;
	const struct a8290_config *config = a8290->config;
	int ret;

	u8 data = 0x81;

	if (config->i2c_txrx_ctl)
		data |= 0x20;

	switch (tone) {
	case SEC_TONE_ON:
		data |= 0x02;
		break;
	case SEC_TONE_OFF:
		break;
	default:
		return -EINVAL;
	}
	ret = a8290_wr(a8290, &data, 1);
	if (ret)
		goto err;
	return ret;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
	return ret;
}

static int a8290_sleep(struct dvb_frontend *fe)
{
	struct a8290_state *a8290 = fe->sec_priv;
	u8 data;
	int ret;

	data = 0x00;
	ret = a8290_wr(a8290, &data, 1);
	if (ret)
		goto err;
	data = 0x80;
	ret = a8290_wr(a8290, &data, 1);
	if (ret)
		goto err;
	a8290->sleep = 1;
	return ret;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
	return ret;
}

static void a8290_release(struct dvb_frontend *fe)
{
	a8290_set_voltage(fe, SEC_VOLTAGE_OFF);
	a8290_sleep(fe);
	kfree(fe->sec_priv);
	fe->sec_priv = NULL;
}

struct dvb_frontend *a8290_attach(struct dvb_frontend *fe,
				  const struct a8290_config *config,
				  struct i2c_adapter *i2c)
{
	struct a8290_state *a8290 = NULL;
	u8 reg[2];
	int ret;

	a8290 = kzalloc(sizeof (struct a8290_state), GFP_KERNEL);
	if (!a8290)
		goto error;

	a8290->verbose				= &verbose;
	a8290->config				= config;
	a8290->i2c				= i2c;
	a8290->frontend				= fe;
	a8290->frontend->sec_priv		= a8290;
	a8290->frontend->ops.set_voltage	= a8290_set_voltage;
	a8290->frontend->ops.set_tone		= a8290_set_tone;
	a8290->frontend->ops.release_sec	= a8290_release;

	ret = a8290_rd(a8290, reg, 2);
	if (ret)
		goto error;

	dprintk(FE_ERROR, 1, "Attaching A8290 LNB controller");
	return a8290->frontend;
error:
	kfree(a8290);
	return NULL;
}
EXPORT_SYMBOL(a8290_attach);
MODULE_PARM_DESC(verbose, "Set Verbosity level");
MODULE_AUTHOR("Manu Abraham");
MODULE_DESCRIPTION("A8290 LNB controller");
MODULE_LICENSE("GPL");
