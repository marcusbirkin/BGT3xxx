/*
	CXD2850 Multistandard Broadcast Frontend driver

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

#include "compat.h"
#include <linux/dvb/frontend.h>
#include "dvb_frontend.h"
#include "dvb_math.h"

#include "cxd2850.h"

static unsigned int verbose;
module_param(verbose, int, 0644);

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



enum cxd2850_delsys {
	CXD2850_DVBS2,
	CXD2850_DVBS,
	CXD2850_AUTO
};

enum cxd2850_cr {
	CXD2850_CR_1_4,
	CXD2850_CR_1_3,
	CXD2850_CR_2_5,
	CXD2850_CR_1_2,
	CXD2850_CR_3_5,
	CXD2850_CR_2_3,
	CXD2850_CR_3_4,
	CXD2850_CR_4_5,
	CXD2850_CR_5_6,
	CXD2850_CR_7_8,
	CXD2850_CR_8_9,
	CXD2850_CR_9_10,
	CXD2850_CR_UNKNOWN
};

enum cxd2850_mod {
	CXD2850_QPSK,
	CXD2850_8PSK,
	CXD2850_APSKxx,
	CXD2850_MOD_UNKNOWN
};

enum cxd2850_sm {
	CXD2850_SM_AUTO,
	CXD2850_SM_LOW,
	CXD2850_SM_HIGH,
};

enum cxd2850_state {
	CXD2850_COLD = 1,
	CXD2850_WARM,
};

struct cxd2850_dev {
	struct i2c_adapter		*i2c;
	const struct cxd2850_config	*config;
	struct dvb_frontend		frontend;
	u8				bank;
	u8				buf[257];

	enum cxd2850_state		state;
	enum cxd2850_delsys		delsys;
	u32				*verbose; /* Cached module verbosity */
};


#define M_REG(__x)		(__x & 0xff)
#define M_BANK(__x)		((__x >> 8) & 0xff)
#define GATEWAY	 		0x09

static int cxd2850_wr_regs(struct cxd2850_dev *cxd2850, u32 addr, u8 *data, int len)
{
	const struct cxd2850_config *cfg = cxd2850->config;
	struct i2c_msg msg;
	int ret;

	u8 *buf = cxd2850->buf;
	u8 reg 	= M_REG(addr);
	u8 bank = M_BANK(addr);

	BUG_ON(len > 256);

	if (bank != cxd2850->bank) {
		buf[0] = 0x00;
		buf[1] = bank;

		msg.addr = cfg->dmd_addr;
		msg.flags = 0;
		msg.len = 2;
		msg.buf = buf;

		ret = i2c_transfer(cxd2850->i2c, &msg, 1);
		if (ret == 1) {
			ret = 0;
		} else {
			dprintk(FE_ERROR, 1, "i2c transfer failed, ret:%d reg:0x%02x", ret, reg);
			ret = -EREMOTEIO;
		}
		cxd2850->bank = bank;
	}
	buf[0] = reg;
	memcpy(&buf[1], data, len);

	msg.len = len + 1;
	msg.buf = buf;
	msg.flags = 0;
	msg.addr = cfg->dmd_addr;

	ret = i2c_transfer(cxd2850->i2c, &msg, 1);
	if (ret != 1) {
		dprintk(FE_ERROR, 1, "WR failed ret:%d reg:%02x len:%d", ret, reg, len);
		ret = -EREMOTEIO;
	} else {
		ret = 0;
	}
	return ret;
}

static int cxd2850_rd_regs(struct cxd2850_dev *cxd2850, u32 addr, u8 *data, int len)
{
	const struct cxd2850_config *cfg = cxd2850->config;
	struct i2c_msg msg[2];
	int ret;

	u8 *buf = cxd2850->buf;
	u8 reg 	= M_REG(addr);
	u8 bank = M_BANK(addr);

	BUG_ON(len > 256);

	if (bank != cxd2850->bank) {
		buf[0] = 0x00;
		buf[1] = bank;

		msg[0].addr = cfg->dmd_addr;
		msg[0].flags = 0;
		msg[0].len = 2;
		msg[0].buf = buf;
		ret = i2c_transfer(cxd2850->i2c, &msg[0], 1);
		if (ret == 1) {
			ret = 0;
		} else {
			dprintk(FE_ERROR, 1, "i2c transfer failed, ret:%d reg:0x%02x", ret, reg);
			ret = -EREMOTEIO;
		}
		cxd2850->bank = bank;
	}
	buf[0] = reg;

	msg[0].addr = cfg->dmd_addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = cfg->dmd_addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = buf;

	ret = i2c_transfer(cxd2850->i2c, msg, 2);
	if (ret != 2) {
		dprintk(FE_ERROR, 1, "RD failed ret:%d reg:%02x len:%d", ret, reg, len);
		ret = -EREMOTEIO;
	} else {
		memcpy(data, buf, len);
		ret = 0;
	}
	return ret;
}

static inline int cxd2850_wr_reg(struct cxd2850_dev *cxd2850, u32 reg, u8 val)
{
	return cxd2850_wr_regs(cxd2850, reg, &val, 1);
}

static inline int cxd2850_rd_reg(struct cxd2850_dev *cxd2850, u32 reg, u8 *val)
{
	return cxd2850_rd_regs(cxd2850, reg, val, 1);
}

static int cxd2850_wr_reg_mask(struct cxd2850_dev *cxd2850, u32 reg, u8 val, u8 mask)
{
	int ret;
	u8 tmp;

	if (mask != 0xff) {
		ret = cxd2850_rd_reg(cxd2850, reg, &tmp);
		if (ret)
			return ret;
		val &= mask;
		tmp &= ~mask;
		val |= tmp;
	}
	return cxd2850_wr_reg(cxd2850, reg, val);
}

static int cxd2850_tuner_wr_regs(struct cxd2850_dev *cxd2850, u8 reg, u8 *val, int len)
{
	const struct cxd2850_config *cfg = cxd2850->config;
	int i, ret;
	u8 *buf = cxd2850->buf;
	struct i2c_msg msg = { .addr = cfg->dmd_addr, .flags = 0, .len = len + 3, .buf = buf };

	buf[0] = GATEWAY;
	buf[1] = cfg->tnr_addr << 1;
	buf[2] = reg;
	memcpy(&buf[3], val, len);

	for (i = 0; i < len; i++)
		dprintk(FE_DEBUG, 1, "Reg(0x%02x) = 0x%x", reg + i, val[i]);

	ret = i2c_transfer(cxd2850->i2c, &msg, 1);
	if (ret == 1) {
		ret = 0;
	} else {
		dprintk(FE_ERROR, 1, "i2c wr failed ret:%d reg:%02x len:%d", ret, reg, len);
		ret = -EREMOTEIO;
	}
	return ret;
}

static int cxd2850_tuner_rd_regs(struct cxd2850_dev *cxd2850, u8 reg, u8 *val, int len)
{
	const struct cxd2850_config *cfg = cxd2850->config;
	int ret;
	u8 *buf = cxd2850->buf;

	struct i2c_msg msg[] = {
		{ .addr = cfg->dmd_addr, .flags = 0, 		.len = 2,   .buf = buf },
		{ .addr = cfg->dmd_addr, .flags = I2C_M_RD,     .len = len, .buf = val }
	};
	buf[0] = GATEWAY;
	buf[1] = (cfg->tnr_addr << 1) | I2C_M_RD;

	ret = i2c_transfer(cxd2850->i2c, msg, 2);
	if (ret == 2) {
		ret = 0;
	} else {
		dprintk(FE_ERROR, 1, "gw wr failed ret:%d reg:%02x len:%d", ret, reg, len);
		ret = -EREMOTEIO;
	}
	return ret;
}

static inline int cxd2850_tuner_wr_reg(struct cxd2850_dev *cxd2850, u8 reg, u8 val)
{
	return cxd2850_tuner_wr_regs(cxd2850, reg, &val, 1);
}

static inline int cxd2850_tuner_rd_reg(struct cxd2850_dev *cxd2850, u8 reg, u8 *val)
{
	return cxd2850_tuner_rd_regs(cxd2850, reg, val, 1);
}

static int cxd2850_tuner_sleep(struct cxd2850_dev *cxd2850)
{
	int ret = 0;

	if (cxd2850->config->shared_xtal) {
		ret = cxd2850_tuner_wr_reg(cxd2850, 0x1a, 0x4c);
		if (ret)
			goto err;
		ret = cxd2850_tuner_wr_reg(cxd2850, 0x16, 0x00);
		if (ret)
			goto err;
		ret = cxd2850_tuner_wr_reg(cxd2850, 0x15, 0x00);
		if (ret)
			goto err;
	} else {
		ret = cxd2850_tuner_wr_reg(cxd2850, 0x11, 0x80);
		if (ret)
			goto err;
	}
	return ret;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
	return ret;
}

static int cxd2850_tuner_wake(struct cxd2850_dev *cxd2850)
{
	int ret = 0;

	if (cxd2850->config->shared_xtal) {
		ret = cxd2850_tuner_wr_reg(cxd2850, 0x15, 0x20);
		if (ret)
			goto err;
		ret = cxd2850_tuner_wr_reg(cxd2850, 0x16, 0x40);
		if (ret)
			goto err;
		ret = cxd2850_tuner_wr_reg(cxd2850, 0x1a, 0xcc);
		if (ret)
			goto err;
	} else {
		ret = cxd2850_tuner_wr_reg(cxd2850, 0x11, 0x00);
		if (ret)
			goto err;
	}
	return ret;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
	return ret;
}

static int cxd2850_init_tuner(struct cxd2850_dev *cxd2850)
{
	u8 data = 0;
	int i, ret = 0;
	static const struct cxd2850_tbl *tbl;

	static const struct cxd2850_tbl {
		u8 addr;
		u8 data;
	} tuner_init_tbl[] = {
		/* defaults */
		{ 0x00, 0x08 },
		{ 0x01, 0x98 },
		{ 0x02, 0x00 },
		{ 0x03, 0x00 },
		{ 0x04, 0x00 },
		{ 0x09, 0x00 },
		{ 0x12, 0x66 },
		{ 0x37, 0x86 },
		/* RF setup */
		{ 0x07, 0x1b },
		{ 0x0e, 0xa0 },
		{ 0x06, 0x1b },
		{ 0x13, 0x20 }
	};

	dprintk(FE_DEBUG, 1, " ");

	ret = cxd2850_tuner_rd_reg(cxd2850, 0x05, &data);
	if (ret)
		goto err;
	if (data & 0x80)
		msleep(10);

	for (i = 0; i < ARRAY_SIZE(tuner_init_tbl); i++) {
		tbl = &tuner_init_tbl[i];
		ret = cxd2850_tuner_wr_reg(cxd2850, tbl->addr, tbl->data);
		if (ret)
			goto err;
	}
	ret = cxd2850_tuner_sleep(cxd2850);
	return ret;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
	return ret;
}

static int cxd2850_tuner_setup(struct cxd2850_dev *cxd2850,
			       enum cxd2850_delsys delsys,
			       u32 frequency,
			       u32 srate)
{
	int ret = 0;
	u8 data[5];
	u8 mixdiv = 0;
	u8 mdiv = 0;
	u32 ms = 0;
	u8 gf_ctl = 0;
	u8 fc_lpf = 0;

	dprintk(FE_DEBUG, 1, "Tune frequency:%d srate:%d", frequency, srate);
	frequency = ((frequency + 500) / 1000) * 1000;

	if (frequency > 1175000) {
		mixdiv = 2;
		mdiv = 0;
	} else {
		mixdiv = 4;
		mdiv = 1;
	}

	ms = ((frequency * mixdiv) / 2 + 1000 / 2) / 1000;
	if (ms > 0x7FFF)
		return -EINVAL;

	if (frequency < 975000)
		gf_ctl = 0x3F;
	else if (frequency < 1050000)
		gf_ctl = 0x77;
	else if (frequency < 1150000)
		gf_ctl = 0x73;
	else if (frequency < 1250000)
		gf_ctl = 0x6E;
	else if (frequency < 1350000)
		gf_ctl = 0x6B;
	else if (frequency < 1450000)
		gf_ctl = 0x69;
	else if (frequency < 1600000)
		gf_ctl = 0x65;
	else if (frequency < 1800000)
		gf_ctl = 0x62;
	else if (frequency < 2000000)
		gf_ctl = 0x60;
	else
		gf_ctl = 0x20;

	switch (delsys) {
	case CXD2850_DVBS:
		dprintk(FE_DEBUG, 1, "DVB-S");
		if (srate <= 4300)
			fc_lpf = 5;
		else if (srate <= 10000)
			fc_lpf = (u8)((srate * 47 + (40000-1)) / 40000);
		else
			fc_lpf = (u8)((srate * 27 + (40000-1)) / 40000 + 5);

		if (fc_lpf > 36)
			fc_lpf = 36;
		break;
	case CXD2850_DVBS2:
	case CXD2850_AUTO:
		dprintk(FE_DEBUG, 1, "DVB-S2/Auto");
		if (srate <= 4500)
			fc_lpf = 5;
		else if (srate <= 10000)
			fc_lpf = (u8)((srate * 11 + (10000-1)) / 10000);
		else
			fc_lpf = (u8)((srate * 3 + (5000-1)) / 5000 + 5);

		if (fc_lpf > 36)
			fc_lpf = 36;
		break;
	default:
		dprintk(FE_DEBUG, 1, "Unknown delivery:%d", delsys);
		return -EINVAL;
	}

	data[0] = (u8)((ms >> 7) & 0xFF);
	data[1] = (u8)((ms & 0x7F) << 1);
	data[2] = 0x00;
	data[3] = 0x00;
	data[4] = (u8)(mdiv << 7);

	ret = cxd2850_tuner_wr_regs(cxd2850, 0x00, data, sizeof(data));
	if (ret)
		goto err;
	ret = cxd2850_tuner_wr_reg(cxd2850, 0x09, gf_ctl);
	if (ret)
		goto err;
	ret = cxd2850_tuner_wr_reg(cxd2850, 0x37, (u8)(0x80 | (fc_lpf << 1)));
	if (ret)
		goto err;
	ret = cxd2850_tuner_wr_reg(cxd2850, 0x05, 0x80);
	if (ret)
		goto err;

	msleep(10);
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_demod_sleep(struct cxd2850_dev *cxd2850)
{
	int ret;

	dprintk(FE_DEBUG, 1, " ");
	ret = cxd2850_wr_reg(cxd2850, 0x00E6, 0x01);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg_mask(cxd2850, 0x00FA, 0x0F, 0x0F);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00FE, 0xFF);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00F2, 0x02);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00FC, 0x07);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00A5, 0x03);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_set_srate(struct cxd2850_dev *cxd2850, u32 srate)
{
	u8 data[3];
	u32 val;
	int ret;

	val = ((srate * 16384) + 500) / 1000;
	if (val > 0x0FFFFF) {
		dprintk(FE_ERROR, 1, "regValue=0x%02x, Invalid SRATE, symbolRate=%d", val, srate);
		return -EINVAL;
	}
	data[0] = (u8)((val >> 16) & 0x0F);
	data[1] = (u8)((val >>  8) & 0xFF);
	data[2] = (u8) (val        & 0xFF);

	ret = cxd2850_wr_regs(cxd2850, 0x4420, data, 3);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_set_delsys(struct cxd2850_dev *cxd2850, enum cxd2850_delsys delsys)
{
	int ret;

	switch (delsys) {
	case CXD2850_DVBS2:
		ret = cxd2850_wr_reg(cxd2850, 0x4430, 0x00);
		if (ret)
			goto err;
		ret = cxd2850_wr_reg(cxd2850, 0x00A5, 0x00);
		if (ret)
			goto err;
		break;
	case CXD2850_DVBS:
		ret = cxd2850_wr_reg(cxd2850, 0x4430, 0x00);
		if (ret)
			goto err;
		ret = cxd2850_wr_reg(cxd2850, 0x00A5, 0x01);
		if (ret)
			goto err;
		break;
	case CXD2850_AUTO:
		ret = cxd2850_wr_reg(cxd2850, 0x4430, 0x01);
		if (ret)
			goto err;
		ret = cxd2850_wr_reg(cxd2850, 0x00A5, 0x01);
		if (ret)
			goto err;
		break;
	}
	ret = cxd2850_wr_reg(cxd2850, 0x00FC, 0x17);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00F2, 0x03);
	if (ret)
		goto err;
	/* setup port */
	ret = cxd2850_wr_reg_mask(cxd2850, 0x00FA, 0x00, 0x0F);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00F5, 0x04);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00FE, 0x00);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_tune(struct cxd2850_dev *cxd2850, u32 frequency, u32 srate)
{
	int ret;
	u8 data, isHSMode;
	u32 fck, N;
	s32 tmp, tmp_min, tmp_abs, min_abs;
	s32 f_spur, f_spur_abs;
	s16 Nsf;

	dprintk(FE_DEBUG, 1, " ");

	ret = cxd2850_set_srate(cxd2850, srate);
	if (ret)
		goto err;
	ret = cxd2850_rd_reg(cxd2850, 0x0050, &data);
	if (ret)
		goto err;
	if (data & 0x10)
		isHSMode = 1;
	else
		isHSMode = 0;

	ret = cxd2850_rd_reg(cxd2850, 0x0050, &data);
	if (ret)
		goto err;
	if (data & 0x20) {
		if (isHSMode)
			fck = 97875;
		else
			fck = 65250;
	} else {
		if (isHSMode)
			fck = 96000;
		else
			fck = 64000;
	}
	tmp_min = (s32)frequency;
	for (N = 0; N < 50; N++) {
		tmp	= (s32)(N * fck) - (s32)frequency;
		tmp_abs = (tmp     >= 0) ? tmp     : (tmp    * (-1));
		min_abs = (tmp_min >= 0) ? tmp_min : (tmp_min * (-1));
		if (min_abs >= tmp_abs)
			tmp_min = tmp;
		else
			break;
	}
	f_spur = tmp_min;
	ret = cxd2850_rd_reg(cxd2850, 0x00D0, &data);
	if (ret)
		goto err;
	f_spur_abs = (f_spur >= 0) ? f_spur : (f_spur * (-1));

	if ((f_spur_abs <= (s32)(srate / 2)) && ((data & 0x01) == 0x01)) {
		Nsf = (s16)(f_spur * 65536 / (s32)fck);

		ret = cxd2850_wr_reg(cxd2850, 0x4520, 0x00);
		if (ret)
			goto err;
		ret = cxd2850_wr_reg(cxd2850, 0x4522, 0x0C);
		if (ret)
			goto err;
		ret = cxd2850_wr_reg(cxd2850, 0x4523, (u8)((u8)((u16)Nsf >> 8) & 0xFF));
		if (ret)
			goto err;
		ret = cxd2850_wr_reg(cxd2850, 0x4524, (u8)((u16)Nsf & 0xFF));
		if (ret)
			goto err;
	} else {
		ret = cxd2850_wr_reg(cxd2850, 0x4520, 0x01);
		if (ret)
			goto err;
	}
	ret = cxd2850_wr_reg(cxd2850, 0x00FF, 0x01);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00E6, 0x00);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_retune(struct cxd2850_dev *cxd2850, u32 frequency, u32 srate)
{
	int ret;

	ret = cxd2850_wr_reg(cxd2850, 0x00E6, 0x01);
	if (ret)
		goto err;
	ret = cxd2850_tune(cxd2850, frequency, srate);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_read_delsys(struct cxd2850_dev *cxd2850, enum cxd2850_delsys *delsys)
{
	int ret;
	u8 data  = 0;

	ret = cxd2850_rd_reg(cxd2850, 0x0011, &data);
	if (ret)
		goto err;
	if (!(data & 0x04))
		return -EINVAL;
	ret = cxd2850_rd_reg(cxd2850, 0x0050, &data);
	if (ret)
		goto err;

	if (data & 0x01)
		*delsys = CXD2850_DVBS;
	else
		*delsys = CXD2850_DVBS2;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_read_dvbs_cnr(struct cxd2850_dev *cxd2850, s16 *cnr)
{
	int ret;
	u8 data[3];
	u32 CPM_QUICKCNDT;
	s32 value;
	s32 cnt;

	static const struct cntable_s_tag {
		s32 val;
		u32 cn;
	} cns[] = {
		{ -130, 1000 },
		{ -123,  900 },
		{ -114,  800 },
		{ -106,  700 },
		{ - 99,  600 },
		{ - 92,  500 },
		{ - 87,  400 },
		{ - 82,  300 },
		{ - 78,  200 },
		{ - 72,  100 },
		{ - 69,    0 },
	};

	dprintk(FE_DEBUG, 1, " ");
	ret = cxd2850_rd_regs(cxd2850, 0x1210, data, 3);
	if (ret)
		goto err;

	if ((data[0] & 0x01) == 0x00) {
		dprintk(FE_ERROR, 1, "not yet ready, reg=%d", data[0]);
		return -EAGAIN;
	}
	CPM_QUICKCNDT = ((u32)(data[1] & 0x1F) << 8) | (u32)data[2];
	value = (s32)(intlog10(CPM_QUICKCNDT) - intlog10(4096));

	if (value <= cns[0].val) {
		*cnr = (s16)((value * (-10)) - 300);
	} else {
		for(cnt = 0; cnt < (s32)(sizeof(cns)/sizeof(cns[0])); cnt++) {
			if (value < cns[cnt].val) {
				 *cnr = (s16)((value - cns[cnt].val) * ((s32)cns[cnt-1].cn - (s32)cns[cnt].cn) / (cns[cnt-1].val - cns[cnt].val) + (s32)cns[cnt].cn);
			}
		}
		*cnr = 0;
	}
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_read_dvbs2_cnr(struct cxd2850_dev *cxd2850, s16 *cnr)
{
	int ret;
	u8 data[3];
	u32 CPM_QUICKCNDT;
	s32 value;
	s32 cnt;

	static const struct cntable_s2_tag{
		s32 val;
		u32 cn;
	} cns2[] = {
		{ -141, 1100 },
		{ -132, 1000 },
		{ -122,  900 },
		{ -113,  800 },
		{ -104,  700 },
		{ - 95,  600 },
		{ - 86,  500 },
		{ - 77,  400 },
		{ - 68,  300 },
		{ - 60,  200 },
		{ - 52,  100 },
		{ - 45,    0 },
	};

	dprintk(FE_DEBUG, 1, " ");
	ret = cxd2850_rd_regs(cxd2850, 0x1210, data, 3);
	if (ret)
		goto err;

	if ((data[0] & 0x01) == 0x00) {
		dprintk(FE_ERROR, 1, "not yet ready, reg=%d", data[0]);
		return -EAGAIN;
	}
	CPM_QUICKCNDT = ((u32)(data[1] & 0x1F) << 8) | (u32)data[2];
	value = (s32)(intlog10(CPM_QUICKCNDT) - intlog10(4096));

	if (value <= cns2[0].val) {
		*cnr = (s16)((value * (-10)) - 300);
	} else {
		for (cnt = 0; cnt < (s32)(sizeof(cns2)/sizeof(cns2[0])); cnt++) {
			if (value < cns2[cnt].val) {
				*cnr = (s16)((value - cns2[cnt].val) * ((s32)cns2[cnt-1].cn - (s32)cns2[cnt].cn) / (cns2[cnt-1].val - cns2[cnt].val)  + (s32)cns2[cnt].cn);
			}
		}
		*cnr = 0;
	}
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_read_cnr(struct dvb_frontend *fe, u16 *snr)
{
	struct cxd2850_dev *cxd2850 = fe->demodulator_priv;
	enum cxd2850_delsys delsys;
	int ret;

	ret = cxd2850_read_delsys(cxd2850, &delsys);
	if (ret)
		goto err;
	switch (delsys) {
	case CXD2850_DVBS:
		dprintk(FE_ERROR, 1, "DVB-S Mode");
		ret = cxd2850_read_dvbs_cnr(cxd2850, snr);
		if (ret)
			goto err;
		break;
	case CXD2850_DVBS2:
		dprintk(FE_ERROR, 1, "DVB-S2 Mode");
		ret = cxd2850_read_dvbs2_cnr(cxd2850, snr);
		if (ret)
			goto err;
		break;
	default:
		dprintk(FE_ERROR, 1, "Invalid Mode");
		return -EINVAL;
	}
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_read_unc_dvbs(struct cxd2850_dev *cxd2850, u32 *unc)
{
	int ret;
	u8 data[4];
	u32 berc, frmc;
	u32 tmp_div;
	u32 tmp_Q;
	u32 tmp_R;

	*unc = 0;

	ret = cxd2850_rd_reg(cxd2850, 0x0025, data);
	if (ret)
		goto err;

	if (data[0] & 0x01) {
		ret = cxd2850_rd_regs(cxd2850, 0x002c, &data[1], 3);
		if (ret)
			goto err;
		berc = (((u32) data[1] & 0xff) << 16) |
		       (((u32) data[2] & 0xff) <<  8) |
		        ((u32) data[3] & 0xff);

		ret = cxd2850_rd_reg(cxd2850, 0x00ba, data);
		if (ret)
			goto err;
		frmc = (u32) (1 << (data[0] & 0xff));
		tmp_div = frmc * 102;
		if ((tmp_div == 0) || (berc > (frmc * 13056))) {
			dprintk(FE_ERROR, 1, "BER too high");
			return -EIO;
		}
		tmp_Q = (berc * 125) / tmp_div;
		tmp_R = (berc * 125) % tmp_div;

		tmp_R *= 625;
		tmp_Q = (tmp_Q * 625) + (tmp_R / tmp_div);
		tmp_R = tmp_R % tmp_div;

		tmp_R *= 100;
		tmp_Q = (tmp_Q * 100) + (tmp_R / tmp_div);
		tmp_R = tmp_R % tmp_div;

		if(tmp_R >= (tmp_div/2))
			*unc= tmp_Q + 1;
		else
			*unc = tmp_Q;
	}
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_read_dvbs_ber(struct cxd2850_dev *cxd2850, u32 *ber)
{
	int ret;
	u8 data[4];
	u32 ber_cnt;
	u32 frm_cnt;
	u32 tmp_div;
	u32 tmp_Q;
	u32 tmp_R;

	ret = cxd2850_rd_regs(cxd2850, 0x0025, &data[0], 4);
	if (ret)
		goto err;

	if ((data[0] & 0x01) == 0) {
		dprintk(FE_ERROR, 1, "Invalid");
		return -EINVAL;
	}
	ber_cnt = (((u32)data[1] & 0xFF) << 16) |
			(((u32)data[2] & 0xFF) <<  8) |
			((u32)data[3] & 0xFF);

	ret = cxd2850_rd_reg(cxd2850, 0x00BA, data);
	if (ret)
		goto err;

	frm_cnt = (u32) (1 << (data[0] & 0x0F));
	tmp_div = frm_cnt * 102;
	if ((tmp_div == 0) || (ber_cnt > (frm_cnt * 13056))) {
		dprintk(FE_ERROR, 1, "BER too high");
		return -EIO;
	}
	tmp_Q = (ber_cnt * 125) / tmp_div;
	tmp_R = (ber_cnt * 125) % tmp_div;

	tmp_R *= 625;
	tmp_Q = (tmp_Q * 625) + (tmp_R / tmp_div);
	tmp_R = tmp_R % tmp_div;

	tmp_R *= 100;
	tmp_Q = (tmp_Q * 100) + (tmp_R / tmp_div);
	tmp_R = tmp_R % tmp_div;

	if(tmp_R >= (tmp_div/2))
		*ber = tmp_Q + 1;
	else
		*ber = tmp_Q;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}
#if 0
static int cxd2850_read_dvbs_fec(struct cxd2850_dev *cxd2850,
				 enum cxd2850_cr *cr,
				 enum cxd2850_mod *mod)
{
	u8 data;
	int ret;

	ret = cxd2850_rd_reg(cxd2850, 0x0011, &data);
	if (ret)
		goto err;
	if (!(data & 0x04)) { /* check TS lock */
		dprintk(FE_ERROR, 1, "Demod Unlocked");
		return -EINVAL;
	}
	ret = cxd2850_rd_reg(cxd2850, 0x2410, &data);
	if (ret)
		goto err;

	/* Check for Code Rate validity */
	if ((data & 0x80) == 0x00) {
		dprintk(FE_ERROR, 1, "Invalid Code Rate");
		return -EIO;
	}
	switch (data & 0x07) {
	case 0x00:
		*cr = CXD2850_CR_1_2;
		break;
	case 0x01:
		*cr = CXD2850_CR_2_3;
		break;
	case 0x02:
		*cr = CXD2850_CR_3_4;
		break;
	case 0x03:
		*cr = CXD2850_CR_5_6;
		break;
	case 0x04:
		*cr = CXD2850_CR_7_8;
		break;
	default:
		return -EIO;
	}
	/* Modulation is always QPSK for DVB-S */
	*mod = CXD2850_QPSK;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}
#endif
static int cxd2850_read_dvbs2_fec(struct cxd2850_dev *cxd2850,
				  enum cxd2850_cr *cr,
				  enum cxd2850_mod *mod)
{
	int ret;
	u8 data;
	u8 mdcd_typ;

	ret = cxd2850_rd_reg(cxd2850, 0x0011, &data);
	if (ret)
		goto err;
	if (!(data & 0x04)) {
		dprintk(FE_ERROR, 1, "Demod Unlocked");
		return -EINVAL;
	}
	ret = cxd2850_rd_reg(cxd2850, 0x0021, &data);
	if (ret)
		goto err;

	mdcd_typ = (u8)(((u32)data >> 2) & 0x1F);

	if (mdcd_typ >= 0x1D) {
		*cr = CXD2850_CR_UNKNOWN;
		*mod = CXD2850_MOD_UNKNOWN;
	} else if (mdcd_typ >= 0x12) {
		*cr = CXD2850_CR_UNKNOWN;
		*mod = CXD2850_APSKxx;
	} else if (mdcd_typ >= 0x0C) {
		*mod = CXD2850_8PSK;
		switch (mdcd_typ) {
		case 0x0C:
			*cr = CXD2850_CR_3_5;
			break;
		case 0x0D:
			*cr = CXD2850_CR_2_3;
			break;
		case 0x0E:
			*cr = CXD2850_CR_3_4;
			break;
		case 0x0F:
			*cr = CXD2850_CR_5_6;
			break;
		case 0x10:
			*cr = CXD2850_CR_8_9;
			break;
		case 0x11:
			*cr = CXD2850_CR_9_10;
			break;
		default:
			return -EIO;
		}
	} else if (mdcd_typ >= 0x01) {
		*mod = CXD2850_QPSK;

		switch (mdcd_typ) {
		case 0x01:
			*cr = CXD2850_CR_1_4;
			break;
		case 0x02:
			*cr = CXD2850_CR_1_3;
			break;
		case 0x03:
			*cr = CXD2850_CR_2_5;
			break;
		case 0x04:
			*cr = CXD2850_CR_1_2;
			break;
		case 0x05:
			*cr = CXD2850_CR_3_5;
			break;
		case 0x06:
			*cr = CXD2850_CR_2_3;
			break;
		case 0x07:
			*cr = CXD2850_CR_3_4;
			break;
		case 0x08:
			*cr = CXD2850_CR_4_5;
			break;
		case 0x09:
			*cr = CXD2850_CR_5_6;
			break;
		case 0x0A:
			*cr = CXD2850_CR_8_9;
			break;
		case 0x0B:
			*cr = CXD2850_CR_9_10;
			break;
		default:
			return -EIO;
		}
	} else {
		*cr = CXD2850_CR_UNKNOWN;
		*mod = CXD2850_MOD_UNKNOWN;
	}
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_read_dvbs2_ber(struct cxd2850_dev *cxd2850, u32 *ber)
{
	int ret;
	u8 data[4];
	enum cxd2850_cr cr;
	enum cxd2850_mod mod;
	u32 ber_cnt;
	u32 frm_cnt;
	u32 temp_a;
	u32 temp_b;
	u32 tmp_div;
	u32 tmp_Q;
	u32 tmp_R;

	ret = cxd2850_rd_regs(cxd2850, 0x002F, &data[0], 4);
	if (ret)
		goto err;

	if((data[0] & 0x01) == 0)
		return -EIO;

	ber_cnt = (((u32)data[1] & 0x7F) << 16)	|
		  (((u32)data[2] & 0xFF) <<  8)	|
		   ((u32)data[3] & 0xFF);

	ret = cxd2850_rd_reg(cxd2850, 0xBC, data);
	if (ret)
		goto err;

	frm_cnt = ((u32) 1 << (data[0] & 0x0F));
	ret = cxd2850_read_dvbs2_fec(cxd2850, &cr, &mod);
	if (ret)
		goto err;

	switch (cr) {
	case CXD2850_CR_1_4:
		temp_a = 1; temp_b =  4;
		break;
	case CXD2850_CR_1_3:
		temp_a = 1; temp_b =  3;
		break;
	case CXD2850_CR_2_5:
		temp_a = 2; temp_b =  5;
		break;
	case CXD2850_CR_1_2:
		temp_a = 1; temp_b =  2;
		break;
	case CXD2850_CR_3_5:
		temp_a = 3; temp_b =  5;
		break;
	case CXD2850_CR_2_3:
		temp_a = 2; temp_b =  3;
		break;
	case CXD2850_CR_3_4:
		temp_a = 3; temp_b =  4;
		break;
	case CXD2850_CR_4_5:
		temp_a = 4; temp_b =  5;
		break;
	case CXD2850_CR_5_6:
		temp_a = 5; temp_b =  6;
		break;
	case CXD2850_CR_8_9:
		temp_a = 8; temp_b =  9;
		break;
	case CXD2850_CR_9_10:
		temp_a = 9; temp_b = 10;
		break;
	default:
		dprintk(FE_ERROR, 1, "Invalid Code Reate");
		return -EIO;
	}
	tmp_div = frm_cnt * 81 * temp_a;

	if (tmp_div == 0) {
		dprintk(FE_ERROR, 1, "Invalid, No Frames ?");
		return -EIO;
	}
	if ((frm_cnt * 6480  * temp_a) < 8388607) {
		if ((ber_cnt * temp_b) > (frm_cnt * 64800 * temp_a))
			return -EIO;
	}

	tmp_Q = (ber_cnt * 25 * temp_b) / tmp_div;
	tmp_R = (ber_cnt * 25 * temp_b) % tmp_div;

	tmp_R *= 100;
	tmp_Q = (tmp_Q * 100) + (tmp_R / tmp_div);
	tmp_R = tmp_R % tmp_div;

	tmp_R *= 50;
	tmp_Q = (tmp_Q * 50) + (tmp_R / tmp_div);
	tmp_R = tmp_R % tmp_div;

	tmp_R *= 10;
	tmp_Q = (tmp_Q * 10) + (tmp_R / tmp_div);
	tmp_R = tmp_R % tmp_div;

	if (tmp_R >= (tmp_div/2))
		*ber = tmp_Q + 1;
	else
		*ber = tmp_Q;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct cxd2850_dev *cxd2850 = fe->demodulator_priv;
	enum cxd2850_delsys delsys;
	int ret;

	ret = cxd2850_read_delsys(cxd2850, &delsys);
	if (ret)
		goto err;
	switch(delsys) {
	case CXD2850_DVBS:
		dprintk(FE_DEBUG, 1, "DVB-S Mode");
		ret = cxd2850_read_dvbs_ber(cxd2850, ber);
		if (ret)
			goto err;
		break;
	case CXD2850_DVBS2:
		dprintk(FE_DEBUG, 1, "DVB-S2 Mode");
		ret = cxd2850_read_dvbs2_ber(cxd2850, ber);
		if (ret)
			goto err;
		break;
	default:
		dprintk(FE_ERROR, 1, "Invalid Mode");
		return -EINVAL;
	}
err:
	return ret;
}

static int cxd2850_read_unc(struct dvb_frontend *fe, u32 *unc)
{
	struct cxd2850_dev *cxd2850 =fe->demodulator_priv;
	enum cxd2850_delsys delsys;
	int ret;

	delsys = CXD2850_DVBS;
	switch (delsys) {
	case CXD2850_DVBS:
		ret = cxd2850_read_unc_dvbs(cxd2850, unc);
		if (ret)
			goto err;
		break;
	case CXD2850_DVBS2:
		break;
	default:
		dprintk(FE_ERROR, 1, "Invalid Mode");
		return -EINVAL;
	}
err:
	return ret;
}

static int cxd2850_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct cxd2850_dev *cxd2850 = fe->demodulator_priv;
	int ret;
	u8 data[2];

	*strength = 0;
	ret = cxd2850_rd_regs(cxd2850, 0x0010, data, 2);
	if (ret)
		goto err;

	if (data[0] & 0x20) {
		ret = cxd2850_rd_regs(cxd2850, 0x001F, &data[0], 2);
		if (ret)
			goto err;
		*strength = (u16)((u16)((u16)(data[0] & 0x1F) << 8) | (u16)data[1]);
		dprintk(FE_DEBUG, 1, "Strength=%d", *strength);
	} else {
		dprintk(FE_ERROR, 1, "AGC unlocked!");
		ret = -EIO;
	}
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

#define SEC_REPEAT	1

static int cxd2850_send_diseqc_msg(struct dvb_frontend *fe, struct dvb_diseqc_master_cmd *cmd)
{
	struct cxd2850_dev *cxd2850 = fe->demodulator_priv;
	int i, loops, ret=0, rem, indx=0;

	loops = cmd->msg_len/12;
	rem   = cmd->msg_len%12;
	if (rem)
		loops += 1;

	ret = cxd2850_wr_reg(cxd2850, 0xFB33, 0x01);
	if (ret)
		goto err;

	for (i = 0; i < loops; i++) {
		if (i != loops-1) {
			ret = cxd2850_wr_reg(cxd2850, 0xFB3D, 12);
			if (ret)
				goto err;
			ret = cxd2850_wr_regs(cxd2850, 0xFB3E, &cmd->msg[indx], 12);
			if (ret)
				goto err;
			ret = cxd2850_wr_reg(cxd2850, 0xFB37, SEC_REPEAT);
			if (ret)
				goto err;
			indx += 12;
		} else {
			ret = cxd2850_wr_reg(cxd2850, 0xFB3D, rem);
			if (ret)
				goto err;
			ret = cxd2850_wr_regs(cxd2850, 0xFB3E, &cmd->msg[indx], rem);
			if (ret)
				goto err;
			ret = cxd2850_wr_reg(cxd2850, 0xFB37, SEC_REPEAT);
			if (ret)
				goto err;
			indx += rem;
			WARN_ON(indx > cmd->msg_len);
		}
	}

	dprintk(FE_DEBUG, 1, "SEC disabled");
	ret = cxd2850_wr_reg(cxd2850, 0xFB33, 0x00);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_idle(struct cxd2850_dev *cxd2850, u8 time)
{
	int ret;
	u8 data[2];
	u16 odiseqc_ckdiv;
	u32 clk;
	u32 reg;

	dprintk(FE_DEBUG, 1, " ");
	ret = cxd2850_rd_regs(cxd2850, 0xFB30, data, 2);
	if (ret)
		goto err;

	odiseqc_ckdiv = (u16)((u16)(data[0] & 0x0F) << 8) | (u16)(data[1] & 0xFF);

	if (odiseqc_ckdiv == 0) {
		ret = -EIO;
		goto err;
	}
	clk = (u32)(((16312500 / odiseqc_ckdiv) + 500) / 1000);
	reg = time * clk;

	if (reg > 0xFFF) {
		ret = -EINVAL;
		goto err;
	}
	data[0] = (u8)((reg >> 8) & 0x0F);
	data[1] = (u8)(reg & 0xFF);

	ret = cxd2850_wr_regs(cxd2850, 0xFB39, data, 2);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
static int cxd2850_send_diseqc_burst(struct dvb_frontend *fe, enum fe_sec_mini_cmd burst)
#else
static int cxd2850_send_diseqc_burst(struct dvb_frontend *fe, fe_sec_mini_cmd_t burst)
#endif
{
	struct cxd2850_dev *cxd2850 = fe->demodulator_priv;
	int ret;

	dprintk(FE_DEBUG, 1, " ");
	ret = cxd2850_wr_reg(cxd2850, 0xFB34, 0x01);
	if (ret)
		goto err;

	ret = cxd2850_wr_reg(cxd2850, 0xFB35, burst == SEC_MINI_A ? 0x00 : 0x01);
	if (ret)
		goto err;
	ret = cxd2850_idle(cxd2850, 15);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
static int cxd2850_set_tone(struct dvb_frontend *fe, enum fe_sec_tone_mode tone)
#else
static int cxd2850_set_tone(struct dvb_frontend *fe, fe_sec_tone_mode_t tone)
#endif
{
	struct cxd2850_dev *cxd2850 = fe->demodulator_priv;
	int ret;

	ret = cxd2850_wr_reg(cxd2850, 0xFB36, tone == SEC_TONE_ON ? 0x01 : 0x00);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_init(struct dvb_frontend *fe)
{
	struct cxd2850_dev *cxd2850 = fe->demodulator_priv;
	const struct cxd2850_config *cfg = cxd2850->config;
	int ret;

	dprintk(FE_DEBUG, 1, " ");
	if (cfg->serial_ts) {
		ret = cxd2850_wr_reg_mask(cxd2850, 0x00F5, 0x04, 0x04);
		if (ret)
			goto err;
		ret = cxd2850_wr_reg(cxd2850, 0x1110, 0x01);
		if (ret)
			goto err;
	} else {
		ret = cxd2850_wr_reg_mask(cxd2850, 0x00F5, 0x00, 0x04);
		if (ret)
			goto err;
		ret = cxd2850_wr_reg(cxd2850, 0x1110, 0x00);
		if (ret)
			goto err;
	}
	if (cfg->serial_d0) {
		ret = cxd2850_wr_reg_mask(cxd2850, 0x00F5, 0x00, 0x01);
		if (ret)
			goto err;
	} else {
		ret = cxd2850_wr_reg_mask(cxd2850, 0x00F5, 0x01, 0x01);
		if (ret)
			goto err;
	}
	if (cfg->tsclk_pol) {
		ret = cxd2850_wr_reg_mask(cxd2850, 0x007F, 0x01, 0x01);
		if (ret)
			goto err;
	} else {
		ret = cxd2850_wr_reg_mask(cxd2850, 0x007F, 0x00, 0x01);
		if (ret)
			goto err;
	}
	if (cfg->tone_out) {
		ret = cxd2850_wr_reg_mask(cxd2850, 0xFB75, 0x00, 0x01);
		if (ret)
			goto err;
	} else {
		ret = cxd2850_wr_reg_mask(cxd2850, 0xFB75, 0x01, 0x01);
		if (ret)
			goto err;
	}
	ret = cxd2850_wr_reg(cxd2850, 0x0002, 0x00);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00FF, 0x01);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00E6, 0x01);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00F2, 0x02);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00FC, 0x07);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00A5, 0x03);
	if (ret)
		goto err;
	ret = cxd2850_init_tuner(cxd2850);
	if (ret)
		goto err;

	ret = cxd2850_wr_reg(cxd2850, 0x00B9, 0x01);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00D0, 0x01);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00D1, 0x02);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00D6, 0x01);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00D7, 0x01);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00ED, 0x05);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00EE, 0x00);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x00EF, 0x00);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x3192, 0x01);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0x651D, 0x00);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg_mask(cxd2850, 0x15AC, 0x00, 0x01);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg(cxd2850, 0xFB6F, 0x01);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static void cxd2850_release(struct dvb_frontend *fe)
{
	struct cxd2850_dev *cxd2850 = fe->demodulator_priv;

	dprintk(FE_DEBUG, 1, " ");
	kfree(cxd2850);
}

static int cxd2850_sleep(struct dvb_frontend *fe)
{
	struct cxd2850_dev *cxd2850 = fe->demodulator_priv;
	int ret;

	dprintk(FE_DEBUG, 1, " ");
	ret = cxd2850_tuner_sleep(cxd2850);
	if (ret)
		goto err;
	ret = cxd2850_demod_sleep(cxd2850);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2850_set_sampling(struct cxd2850_dev *cxd2850, enum cxd2850_sm mode)
{
	int ret;

	switch (mode) {
	case CXD2850_SM_AUTO:
		dprintk(FE_DEBUG, 1, "SM_AUTO");
		ret = cxd2850_wr_reg_mask(cxd2850, 0x00D0, 0x01, 0x01);
		if (ret)
			goto err;
		break;
	case CXD2850_SM_LOW:
		dprintk(FE_DEBUG, 1, "SM_LOW");
		ret = cxd2850_wr_reg_mask(cxd2850, 0x00D0, 0x00, 0x01);
		if (ret)
			goto err;
		ret = cxd2850_wr_reg(cxd2850, 0x00F3, 0x01);
		if (ret)
			goto err;
		break;
	case CXD2850_SM_HIGH:
		dprintk(FE_DEBUG, 1, "SM_HIGH");
		ret = cxd2850_wr_reg_mask(cxd2850, 0x00D0, 0x00, 0x01);
		if (ret)
			goto err;
		ret = cxd2850_wr_reg(cxd2850, 0x00F3, 0x11);
		if (ret)
			goto err;
		break;
	default:
		dprintk(FE_ERROR, 1, "Invalid Sampling Mode");
		break;
	}
err:
	return ret;
}

static enum dvbfe_algo cxd2850_frontend_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_CUSTOM;
}

static int cxd2850_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	struct cxd2850_dev *cxd2850 = fe->demodulator_priv;
	int ret;
	u8 data[2];

	*status = 0;

	ret = cxd2850_rd_regs(cxd2850, 0x0010, data, 2);
	if (ret)
		goto err;

	if (data[0] & 0x20)
		*status = FE_HAS_SIGNAL | FE_HAS_CARRIER;

	if (data[1] & 0x04) {
		dprintk(FE_DEBUG, 1, "TS Lock acquired");
		*status = FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
	}
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static enum dvbfe_search cxd2850_search(struct dvb_frontend *fe)
{
	struct cxd2850_dev *cxd2850		= fe->demodulator_priv;
	struct dtv_frontend_properties *props	= &fe->dtv_property_cache;
	enum cxd2850_delsys delsys		= props->delivery_system;

	u32 srate;
	u8 ts_lock = 0;
	int i, ret;

	dprintk(FE_DEBUG, 1, " ");
	srate  = props->symbol_rate / 1000;
	delsys = CXD2850_AUTO;

	if (!props->frequency)
		return DVBFE_ALGO_SEARCH_INVALID;

	ret = cxd2850_tuner_wake(cxd2850);
	if (ret)
		goto err;
	ret = cxd2850_tuner_setup(cxd2850, delsys, props->frequency, srate);
	if (ret)
		goto err;
	ret = cxd2850_set_sampling(cxd2850, CXD2850_SM_AUTO);
	if (ret)
		goto err;
	ret = cxd2850_wr_reg_mask(cxd2850, 0x4424, 0x00, 0x01);
	if (ret)
		goto err;
	if (cxd2850->delsys != delsys) {
		ret = cxd2850_demod_sleep(cxd2850);
		if (ret)
			goto err;
		cxd2850->state = CXD2850_COLD;
	}
	if (cxd2850->state == CXD2850_WARM) {
		ret = cxd2850_retune(cxd2850, props->frequency, srate);
		if (ret)
			goto err;
	} else {
		ret = cxd2850_set_delsys(cxd2850, delsys);
		if (ret)
			goto err;
		ret = cxd2850_tune(cxd2850, props->frequency, srate);
		if (ret)
			goto err;
	}
	cxd2850->delsys = delsys;

	for (i = 0; i < 10; i++) {
		msleep(100);
		ret = cxd2850_rd_reg(cxd2850, 0x0011, &ts_lock);
		if (ret)
			goto err;
		if (ts_lock & 0x04)
			return DVBFE_ALGO_SEARCH_SUCCESS;
	}
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return DVBFE_ALGO_SEARCH_FAILED;
}

static struct dvb_frontend_ops cxd2850_ops = {

	.delsys = { SYS_DVBS, SYS_DVBS2 },

	.info = {
		.name			= "CXD2850 Multistandard",

		.frequency_min		= 950000,
		.frequency_max 		= 2150000,

		.frequency_stepsize	= 0,
		.frequency_tolerance	= 0,

		.symbol_rate_min 	= 1000000,
		.symbol_rate_max 	= 45000000,

		.caps			= FE_CAN_INVERSION_AUTO |
					  FE_CAN_FEC_AUTO       |
					  FE_CAN_QPSK           |
					  FE_CAN_2G_MODULATION
	},

	.release			= cxd2850_release,
	.init				= cxd2850_init,

	.sleep				= cxd2850_sleep,
	.get_frontend_algo		= cxd2850_frontend_algo,

	.set_tone			= cxd2850_set_tone,

	.diseqc_send_master_cmd		= cxd2850_send_diseqc_msg,
	.diseqc_send_burst		= cxd2850_send_diseqc_burst,
//	.diseqc_recv_slave_reply	= cxd2850_recv_slave_reply,

	.search				= cxd2850_search,
	.read_status			= cxd2850_read_status,
	.read_ber			= cxd2850_read_ber,
	.read_signal_strength		= cxd2850_read_signal_strength,
	.read_snr			= cxd2850_read_cnr,
	.read_ucblocks			= cxd2850_read_unc,
};


struct dvb_frontend *cxd2850_attach(const struct cxd2850_config *config,
				    struct i2c_adapter *i2c)
{
	struct cxd2850_dev *cxd2850 = NULL;
	u8 id;
	int ret;

	cxd2850 = kzalloc(sizeof (struct cxd2850_dev), GFP_KERNEL);
	if (!cxd2850)
		goto error;

	cxd2850->verbose			= &verbose;
	cxd2850->config				= config;
	cxd2850->i2c				= i2c;
	cxd2850->frontend.ops			= cxd2850_ops;
	cxd2850->frontend.demodulator_priv	= cxd2850;

	ret = cxd2850_rd_reg(cxd2850, 0xfd, &id);
	if (ret < 0) {
		dprintk(FE_ERROR, 1, "Read error, ret=%d", ret);
	}
	if (id == 0x31)
		dprintk(FE_ERROR, 1, "Found CXD2850A, ID=0x%02x", id);

	dprintk(FE_ERROR, 1, "Attaching CXD2850 demodulator(ID=0x%x)", id);
	return &cxd2850->frontend;
error:
	kfree(cxd2850);
	return NULL;
}
EXPORT_SYMBOL(cxd2850_attach);
MODULE_PARM_DESC(verbose, "Set Verbosity level");
MODULE_AUTHOR("Manu Abraham");
MODULE_DESCRIPTION("CXD2850 Multi-Std Broadcast frontend");
MODULE_LICENSE("GPL");
