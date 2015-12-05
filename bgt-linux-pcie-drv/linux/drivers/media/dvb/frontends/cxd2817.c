/*
	Sony CXD2817 Multistandard Broadcast Frontend driver

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
#include "dvb_math.h"

#include "cxd2817.h"

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

enum cxd2817_delsys {
	CXD2817_NONE,
	CXD2817_DVBC,
	CXD2817_DVBT,
};

enum cxd2817_qam_const {
	CXD2817_QAM16,
	CXD2817_QAM32,
	CXD2817_QAM64,
	CXD2817_QAM128,
	CXD2817_QAM256
};

enum cxd2817_tun_ver {
	NONE,
	A2E,
	A2D,
	A2S,
};

enum cxd2817_bermctl {
	CXD2817_BERC_STOP,
	CXD2817_BERC_START
};

enum cxd2817_state {
	CXD2817_SHUTDOWN,
	CXD2817_SLEEP,
	CXD2817_ACTIVE,
};

static const char *stname[] = {
	"CXD2817_SHUTDOWN",
	"CXD2817_SLEEP",
	"CXD2817_ACTIVE",
};

#define CXD2817_SET_STATE(__x) do {							\
	dprintk(FE_DEBUG, 1, "Current state:%s New state:%s",				\
		stname[cxd2817->state], stname[__x]);					\
	cxd2817->state = __x;								\
} while (0)

struct cxd2817_dev {
	struct i2c_adapter		*i2c;
	struct dvb_frontend		fe;
	const struct cxd2817_config	*cfg;
	u8 				bank;
	u8				buf[257];

	u8				if_shared;
	enum cxd2817_tun_ver		tun_ver;
	enum cxd2817_delsys		delsys;
	enum cxd2817_state		state;
	enum cxd2817_bermctl		berc_run;
	u32				bw;
	bool				gdeq_en;
	bool				rfmon_en;
	u32				*verbose;
};

#define M_REG(__x)		(__x & 0xff)
#define M_ADDR(__x)		(cfg->addr | (((__x >> 16) & 0x1) << 1))
#define M_BANK(__x)		((__x >> 8) & 0xff)
#define COMP_WIDTH(__x)		(0x02 << (__x - 1))
#define MASK_HI(__n)		((__n) == 0 ? 0 : 0xFFFFFFFFU << (32 - (__n)))
#define MASK_LO(__n)		((__n) == 0 ? 0 : 0xFFFFFFFFU >> (32 - (__n)))

static int cxd2817_wr_regs(struct cxd2817_dev *cxd2817, u32 reg, u8 *val, int len)
{
	const struct cxd2817_config *cfg = cxd2817->cfg;
	u8 bank = M_BANK(reg);
	int ret;

	u8 *buf = cxd2817->buf;
	struct i2c_msg msg;

	WARN_ON(len > 256);
	if (bank != cxd2817->bank) {
		buf[0] = 0x00;
		buf[1] = M_BANK(reg);

		msg.len = 1;
		msg.buf = buf;
		msg.flags = 0;
		msg.addr = M_ADDR(reg);
		ret = i2c_transfer(cxd2817->i2c, &msg, 1);
		if (ret != 1) {
			dprintk(FE_ERROR, 1, "i2c transfer failed ret:%d reg:%02x", ret, reg);
			ret = -EREMOTEIO;
		} else {
			ret = 0;
		}
		if (ret)
			return ret;
		cxd2817->bank = bank;
	}
	buf[0] = M_REG(reg);
	memcpy(&buf[1], val, len);

	msg.len = len + 1;
	msg.buf = buf;
	msg.flags = 0;
	msg.addr = M_ADDR(reg);

	ret = i2c_transfer(cxd2817->i2c, &msg, 1);
	if (ret != 1) {
		dprintk(FE_ERROR, 1, "WR failed ret:%d reg:%02x len:%d", ret, reg, len);
		ret = -EREMOTEIO;
	} else {
		ret = 0;
	}
	return ret;
}

static int cxd2817_rd_regs(struct cxd2817_dev *cxd2817, u32 reg, u8 *val, int len)
{
	const struct cxd2817_config *cfg = cxd2817->cfg;
	u8 bank = M_BANK(reg);
	int ret;

	u8 *buf = cxd2817->buf;
	struct i2c_msg msg[2];

	WARN_ON(len > 256);
	if (bank != cxd2817->bank) {
		buf[0] = 0x00;
		buf[1] = M_BANK(reg);

		msg[0].addr = M_ADDR(reg);
		msg[0].flags = 0;
		msg[0].len = 1;
		msg[0].buf = buf;

		ret = i2c_transfer(cxd2817->i2c, &msg[0], 1);
		if (ret != 1) {
			dprintk(FE_ERROR, 1, "i2c transfer failed ret:%d reg:%02x", ret, reg);
			ret = -EREMOTEIO;
		} else {
			ret = 0;
		}
		if (ret)
			return ret;
		cxd2817->bank = bank;
	}
	buf[0] = M_REG(reg);

	msg[0].addr = M_ADDR(reg);
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = M_ADDR(reg);
	msg[1].flags = I2C_M_RD;
	msg[1].len = len;
	msg[1].buf = buf;

	ret = i2c_transfer(cxd2817->i2c, msg, 2);
	if (ret != 2) {
		dprintk(FE_ERROR, 1, "RD failed ret:%d reg:%02x len:%d", ret, reg, len);
		ret = -EREMOTEIO;
	} else {
		memcpy(val, buf, len);
		ret = 0;
	}
	return ret;
}

static inline int cxd2817_wr(struct cxd2817_dev *cxd2817, u32 reg, u8 val)
{
	return cxd2817_wr_regs(cxd2817, reg, &val, 1);
}

static inline int cxd2817_rd(struct cxd2817_dev *cxd2817, u32 reg, u8 *val)
{
	return cxd2817_rd_regs(cxd2817, reg, val, 1);
}

static int cxd2817_wr_mask(struct cxd2817_dev *cxd2817, u32 reg, u8 val, u8 mask)
{
	int ret;
	u8 tmp;

	if (mask != 0xff) {
		ret = cxd2817_rd(cxd2817, reg, &tmp);
		if (ret)
			return ret;

		val &= mask;
		tmp &= ~mask;
		val |= tmp;
	}
	return cxd2817_wr(cxd2817, reg, val);
}

static void cxd2817_release(struct dvb_frontend *fe)
{
	struct cxd2817_dev *cxd2817 = fe->demodulator_priv;
	dprintk(FE_DEBUG, 1, "Release");

	kfree(cxd2817);
	return;
}

static int cxd2817_berm_ctl(struct cxd2817_dev *cxd2817, enum cxd2817_bermctl ctl)
{
	int ret;

	switch (cxd2817->delsys) {
	case CXD2817_DVBT:
		ret = cxd2817_wr(cxd2817, 0x00079, ctl);
		if (ret)
			goto err;
		break;
	case CXD2817_DVBC:
		ret = cxd2817_wr(cxd2817, 0x10079, ctl);
		if (ret)
			goto err;
		break;
	case CXD2817_NONE:
	default:
		ret = -EINVAL;
		goto out;
	}
	cxd2817->berc_run = ctl;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2817_dvbt_params(struct cxd2817_dev *cxd2817)
{
	struct dvb_frontend *fe = &cxd2817->fe;
	const struct cxd2817_config *cfg = cxd2817->cfg;

	int ret;
	u32 if_freq, lof, mclk;
	u8 f_aci, f_tsif;
	u8 *t_trl, *t_gdeq, *t_dly, *t_nf1, *t_nf2, *t_nf3;
	u8 f_if[3];

	u8 trl_6[] = { 0x17, 0xEA, 0xAA, 0xAA, 0xAA };
	u8 trl_7[] = { 0x14, 0x80, 0x00, 0x00, 0x00 };
	u8 trl_8[] = { 0x11, 0xF0, 0x00, 0x00, 0x00 };

	u8 dvbt6_a2e_gdeq[] = {
		0x27, 0xA7, 0x28, 0xB3, 0x02, 0xF0, 0x01,
		0xE8, 0x00, 0xCF, 0x00, 0xE6, 0x23, 0xA4
	};
	u8 dvbt6_a2s_gdeq[] = {
		0x19, 0x24, 0x2B, 0xB7, 0x2C, 0xAC, 0x29,
		0xA6, 0x2A, 0x9F, 0x2A, 0x99, 0x2A, 0x9B
	};
	u8 dvbt7_a2e_gdeq[] = {
		0x2C, 0xBD, 0x02, 0xCF, 0x04, 0xF8, 0x23,
		0xA6, 0x29, 0xB0, 0x26, 0xA9, 0x21, 0xA5
	};
	u8 dvbt7_a2s_gdeq[] = {
		0x1B, 0x22, 0x2B, 0xC1, 0x2C, 0xB3, 0x2B,
		0xA9, 0x2B, 0xA0, 0x2B, 0x97, 0x2B, 0x9B
	};
	u8 dvbt8_a2e_gdeq[] = {
		0x26, 0xAF, 0x06, 0xCD, 0x13, 0xBB, 0x28,
		0xBA, 0x23, 0xA9, 0x1F, 0xA8, 0x2C, 0xC8
	};
	u8 dvbt8_a2s_gdeq[] = {
		0x1E, 0x1D, 0x29, 0xC9, 0x2A, 0xBA, 0x29,
		0xAD, 0x29, 0xA4, 0x29, 0x9A, 0x28, 0x9E
	};
	u8 dly_dvbt6[] = { 0x1F, 0xDC };
	u8 dly_dvbt7[] = { 0x12, 0xF8 };
	u8 dly_dvbt8[] = { 0x01, 0xE0 };
	u8 nf1_dvbt6[] = { 0x00, 0x02 };
	u8 nf2_dvbt6[] = { 0x05, 0x05 };
	u8 nf3_dvbt6[] = { 0x91, 0xA0 };
	u8 nf1_dvbt7[] = { 0x01, 0x02 };
	u8 nf2_dvbt7[] = { 0x03, 0x01 };
	u8 nf3_dvbt7[] = { 0x97, 0x00 };
	u8 nf1_dvbt8[] = { 0x01, 0x02 };
	u8 nf2_dvbt8[] = { 0x05, 0x05 };
	u8 nf3_dvbt8[] = { 0x91, 0xA0 };

	if (cxd2817->gdeq_en) {
		ret = cxd2817_wr_mask(cxd2817, 0x00A5, 0x01, 0x01);
		if (ret)
			goto err;
		ret = cxd2817_wr_mask(cxd2817, 0x0082, 0x40, 0x60);
		if (ret)
			goto err;
	} else {
		ret = cxd2817_wr_mask(cxd2817, 0x00A5, 0x00, 0x01);
		if (ret)
			goto err;
		ret = cxd2817_wr_mask(cxd2817, 0x0082, 0x20, 0x60);
		if (ret)
			goto err;
		ret = cxd2817_wr(cxd2817, 0x00C2, 0x11); /* ACI filter */
		if (ret)
			goto err;
	}
	ret = cxd2817_wr(cxd2817, 0x016A, 0x50);
	if (ret)
		goto err;
	ret = cxd2817_wr_mask(cxd2817, 0x0148, 0x10, 0x10);
	if (ret)
		goto err;
	ret = cxd2817_wr(cxd2817, 0x0427, 0x41);
	if (ret)
		goto err;

	if (fe->ops.tuner_ops.set_params) {
		ret = fe->ops.tuner_ops.set_params(fe);
		if (ret)
			goto err;
	}

	switch (cxd2817->bw) {
	case 6000000:
		f_aci	= 0x13;
		t_trl	= trl_6;
		if (cxd2817->tun_ver == A2D || cxd2817->tun_ver == A2E)
			t_gdeq = dvbt6_a2e_gdeq;
		else if (cxd2817->tun_ver == A2S)
			t_gdeq = dvbt6_a2s_gdeq;
		else
			t_gdeq = NULL;
		f_tsif	= 0x80;
		t_dly	= dly_dvbt6;
		t_nf1	= nf1_dvbt6;
		t_nf2	= nf2_dvbt6;
		t_nf3	= nf3_dvbt6;
		break;
	case 7000000:
		f_aci	= 0x13;
		t_trl	= trl_7;
		if (cxd2817->tun_ver == A2D || cxd2817->tun_ver == A2E)
			t_gdeq = dvbt7_a2e_gdeq;
		else if (cxd2817->tun_ver == A2S)
			t_gdeq = dvbt7_a2s_gdeq;
		else
			t_gdeq = NULL;
		f_tsif	= 0x40;
		t_dly	= dly_dvbt7;
		t_nf1	= nf1_dvbt7;
		t_nf2	= nf2_dvbt7;
		t_nf3	= nf3_dvbt7;
		break;
	case 8000000:
		f_aci	= 0x11;
		t_trl	= trl_8;
		if (cxd2817->tun_ver == A2D || cxd2817->tun_ver == A2E)
			t_gdeq = dvbt8_a2e_gdeq;
		else if (cxd2817->tun_ver == A2S)
			t_gdeq = dvbt8_a2s_gdeq;
		else
			t_gdeq = NULL;
		f_tsif	= 0x00;
		t_dly	= dly_dvbt8;
		t_nf1	= nf1_dvbt8;
		t_nf2	= nf2_dvbt8;
		t_nf3	= nf3_dvbt8;
		break;
	default:
		dprintk(FE_ERROR, 1, "Invalid bandwidth, bw=%d", cxd2817->bw);
		ret = -EINVAL;
		goto out;
	}
	if (fe->ops.tuner_ops.get_if_frequency) {
		ret = fe->ops.tuner_ops.get_if_frequency(fe, &if_freq);
		if (ret) {
			dprintk(FE_ERROR, 1, "ERROR: get_if!!");
			goto err;
		}
		if_freq /= 100000;
		mclk = cfg->xtal / 100000;
		lof = if_freq * COMP_WIDTH(24) / mclk;
		f_if[0] = ((lof >> 16) & 0xff);
		f_if[1] = ((lof >>  8) & 0xff);
		f_if[2] = ((lof >>  0) & 0xff);
	} else {
		dprintk(FE_ERROR, 1, "Unable to determine IF!, exiting ..");
		ret = -EIO;
		goto out;
	}
	if (cxd2817->gdeq_en) {
		ret = cxd2817_wr(cxd2817, 0x00c2, f_aci);
		if (ret)
			goto err;
	}

	ret = cxd2817_wr_regs(cxd2817, 0x009f, t_trl, 5);
	if (ret)
		goto err;
	ret = cxd2817_wr_regs(cxd2817, 0x00a6, t_gdeq, 14);
	if (ret)
		goto err;
	ret = cxd2817_wr_regs(cxd2817, 0x00b6, f_if, 3);
	if (ret)
		goto err;
	ret = cxd2817_wr_mask(cxd2817, 0x00d7, f_tsif, 0xc0);
	if (ret)
		goto err;
	ret = cxd2817_wr_regs(cxd2817, 0x00d9, t_dly, 4);
	if (ret)
		goto err;

	if (cxd2817->gdeq_en) {
		ret = cxd2817_wr_regs(cxd2817, 0x0738, t_nf1, 2);
		if (ret)
			goto err;
		ret = cxd2817_wr_regs(cxd2817, 0x073c, t_nf2, 2);
		if (ret)
			goto err;
		ret = cxd2817_wr_regs(cxd2817, 0x0744, t_nf3, 2);
		if (ret)
			goto err;
	}
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2817_dvbt_setup(struct cxd2817_dev *cxd2817)
{
	int ret;

	ret = cxd2817_wr(cxd2817, 0x0080, 0x00);
	if (ret)
		goto err;

	if (cxd2817->rfmon_en) {
		ret = cxd2817_wr(cxd2817, 0x0081, 0x13);
		if (ret)
			goto err;
	} else {
		ret = cxd2817_wr(cxd2817, 0x0081, 0x03);
		if (ret)
			goto err;
	}
	ret = cxd2817_wr(cxd2817, 0x0085, 0x07);
	if (ret)
		goto err;
	if (cxd2817->rfmon_en) {
		ret = cxd2817_wr(cxd2817, 0x0088, 0x02);
		if (ret)
			goto err;
	}
	ret = cxd2817_wr(cxd2817, 0x00070, 0x08); /* serial */
	if (ret)
		goto err;
	ret = cxd2817_wr_mask(cxd2817, 0x000d7, 0x01, 0x01);
	if (ret)
		goto err;
	ret = cxd2817_wr_mask(cxd2817, 0x000d7, 0x02, 0x02);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2817_sleep(struct cxd2817_dev *cxd2817)
{
	int ret = 0;

	ret = cxd2817_berm_ctl(cxd2817, CXD2817_BERC_STOP);
	if (ret)
		goto err;
	ret = cxd2817_wr(cxd2817, 0x00FF, 0x1F);
	if (ret)
		goto err;
	ret = cxd2817_wr(cxd2817, 0x0085, (cxd2817->if_shared ? 0x06 : 0x00));
	if (ret)
		goto err;
	ret = cxd2817_wr(cxd2817, 0x0088, 0x01);
	if (ret)
		goto err;
	ret = cxd2817_wr(cxd2817, 0x0081, 0x00);
	if (ret)
		goto err;
	ret = cxd2817_wr(cxd2817, 0x0080, 0x00);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2817_dvbc_setup(struct cxd2817_dev *cxd2817)
{
	struct dvb_frontend *fe = &cxd2817->fe;
	const struct cxd2817_config *cfg = cxd2817->cfg;

	u32 if_freq, lof, mclk;
	u8 *t_gdeq;
	u8 f_if[2];
	int ret = 0;

	u8 dvbc_a2e_gdeq[] = {
		0x2D, 0xC7, 0x04, 0xF4, 0x07, 0xC5, 0x2A,
		0xB8, 0x27, 0x9E, 0x27, 0xA4, 0x29, 0xAB
	};
	u8 dvbc_a2s_gdeq[] = {
		0x27, 0xD9, 0x27, 0xC5, 0x25, 0xBB, 0x24,
		0xB1, 0x24, 0xA9, 0x23, 0xA4, 0x23, 0xA2
	};

	if (cxd2817->tun_ver == A2D || cxd2817->tun_ver == A2E) {
		t_gdeq = dvbc_a2e_gdeq;
	} else if (cxd2817->tun_ver == A2S) {
		t_gdeq = dvbc_a2s_gdeq;
	} else {
		t_gdeq = NULL;
		ret = -EINVAL;
		goto err;
	}

	ret = cxd2817_wr(cxd2817, 0x0080, 0x01);
	if (ret)
		goto err;
	if (cxd2817->rfmon_en) {
		ret = cxd2817_wr(cxd2817, 0x0081, 0x15);
		if (ret)
			goto err;
	} else {
		ret = cxd2817_wr(cxd2817, 0x0081, 0x05);
		if (ret)
			goto err;
	}
	ret = cxd2817_wr(cxd2817, 0x0085, 0x07);
	if (ret)
		goto err;
	if (cxd2817->rfmon_en) {
		ret = cxd2817_wr(cxd2817, 0x0088, 0x02);
		if (ret)
			goto err;
	}
	if (cxd2817->gdeq_en) {
		dprintk(FE_DEBUG, 1, "GDEQ Enabled");
		ret = cxd2817_wr_mask(cxd2817, 0x0100A5, 0x01, 0x01);
		if (ret)
			goto err;
		ret = cxd2817_wr_mask(cxd2817, 0x000082, 0x40, 0x60);
		if (ret)
			goto err;
	} else {
		dprintk(FE_DEBUG, 1, "GDEQ Disabled");
		ret = cxd2817_wr_mask(cxd2817, 0x0100A5, 0x00, 0x01);
		if (ret)
			goto err;
		ret = cxd2817_wr_mask(cxd2817, 0x000082, 0x20, 0x60);
		if (ret)
			goto err;
	}
	ret = cxd2817_wr_regs(cxd2817, 0x0100A6, t_gdeq, 14);
	if (ret)
		goto err;

	if (fe->ops.tuner_ops.get_if_frequency) {
		ret = fe->ops.tuner_ops.get_if_frequency(fe, &if_freq);
		if (ret) {
			dprintk(FE_ERROR, 1, "ERROR: get_if!!");
			goto err;
		}
		if_freq /= 100000;
		mclk = cfg->xtal / 100000;
		lof = if_freq * COMP_WIDTH(14) / mclk;
		f_if[0] = ((lof >> 8) & 0x3f);
		f_if[1] = ((lof >> 0) & 0xff);
	} else {
		dprintk(FE_ERROR, 1, "Unable to determine IF!, exiting ..");
		ret = -EIO;
		goto out;
	}

	ret = cxd2817_wr_regs(cxd2817, 0x010042, f_if, 2);
	if (ret)
		goto err;

	if (cxd2817->gdeq_en) {
		ret = cxd2817_wr(cxd2817, 0x010053, 0x48);
		if (ret)
			goto err;
	}
	ret = cxd2817_wr(cxd2817, 0x01016A, 0x48);
	if (ret)
		goto err;
	ret = cxd2817_wr(cxd2817, 0x010427, 0x41);
	if (ret)
		goto err;
	ret = cxd2817_wr_mask(cxd2817, 0x010020, 0x06, 0x07);
	if (ret)
		goto err;
	ret = cxd2817_wr(cxd2817, 0x010059, 0x50);
	if (ret)
		goto err;
	ret = cxd2817_wr_mask(cxd2817, 0x010087, 0x0C, 0x3C);
	if (ret)
		goto err;
	ret = cxd2817_wr(cxd2817, 0x01008B, 0x07);
	if (ret)
		goto err;
	ret = cxd2817_wr(cxd2817, 0x10070, 0x08); // serial
	if (ret)
		goto err;
	ret = cxd2817_wr_mask(cxd2817, 0x000d7, 0x01, 0x01);
	if (ret)
		goto err;
	ret = cxd2817_wr_mask(cxd2817, 0x000d7, 0x02, 0x02);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

#define DEF_PIO			0x08

static int cxd2817_tune(struct cxd2817_dev *cxd2817, enum cxd2817_delsys delsys)
{
	struct dvb_frontend *fe = &cxd2817->fe;
	struct dtv_frontend_properties *props	= &fe->dtv_property_cache;
	u32 bw = props->bandwidth_hz;
	int ret = 0;

	if (delsys == CXD2817_DVBT) {
		if (!((bw == 6000000) || (bw == 7000000) || (bw == 8000000))) {
			dprintk(FE_ERROR, 1, "Invalid bandwidth, bw=%d", bw);
			ret = -EINVAL;
			goto out;
		}
		/* setup stream */
	}
	cxd2817->delsys = delsys;
	if (cxd2817->state == CXD2817_ACTIVE && delsys != cxd2817->delsys) {
		dprintk(FE_DEBUG, 1, "Normal 2 Sleep");
		ret = cxd2817_sleep(cxd2817);
		if (ret)
			goto err;
	}
	CXD2817_SET_STATE(CXD2817_SLEEP);
	if (cxd2817->delsys == CXD2817_DVBT) {
		cxd2817->bw = bw;
		dprintk(FE_DEBUG, 1, "Tune DVB-T");
		ret = cxd2817_dvbt_setup(cxd2817);
		if (ret)
			goto err;
		ret = cxd2817_dvbt_params(cxd2817);
		if (ret)
			goto err;
	} else if (cxd2817->delsys == CXD2817_DVBC) {
		ret = cxd2817_dvbc_setup(cxd2817);
		if (ret)
			goto err;
	} else {
		dprintk(FE_ERROR, 1, "Invalid delivery system");
		ret = -EINVAL;
		goto out;
	}
	CXD2817_SET_STATE(CXD2817_ACTIVE);
	ret = cxd2817_wr(cxd2817, 0x00FF, DEF_PIO);
	if (ret)
		goto err;
	ret = cxd2817_wr(cxd2817, 0x00FE, 0x01);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2817_init(struct dvb_frontend *fe)
{
	struct cxd2817_dev *cxd2817 = fe->demodulator_priv;
	int ret = 0;

	dprintk(FE_DEBUG, 1, "Initializing ..");
	cxd2817->gdeq_en = 1;
	cxd2817->tun_ver = A2E;
	cxd2817->if_shared = 0;

	ret = cxd2817_wr(cxd2817, 0x007E, 0x02);
	if (ret)
		goto err;
	msleep(10); /* Wait */
	ret = cxd2817_wr(cxd2817, 0x007F, 0x01);
	if (ret)
		goto err;
	CXD2817_SET_STATE(CXD2817_SLEEP);
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2817_shutdown(struct dvb_frontend *fe)
{
	struct cxd2817_dev *cxd2817 = fe->demodulator_priv;
	int ret = 0;

	if (cxd2817->state == CXD2817_ACTIVE) {
		cxd2817->berc_run = 0;

		ret = cxd2817_wr(cxd2817, 0x00FF, 0x1F);
		if (ret)
			goto err;
		ret = cxd2817_wr(cxd2817, 0x0085, (cxd2817->if_shared ? 0x06 : 0x00));
		if (ret)
			goto err;
		ret = cxd2817_wr(cxd2817, 0x0088, 0x01);
		if (ret)
			goto err;
		ret = cxd2817_wr(cxd2817, 0x0081, 0x00);
		if (ret)
			goto err;
		ret = cxd2817_wr(cxd2817, 0x0080, 0x00);
		if (ret)
			goto err;
	}

	ret = cxd2817_wr(cxd2817, 0x007F, 0x00);
	if (ret)
		goto err;
	ret = cxd2817_wr(cxd2817, 0x007E, 0x03);
	if (ret)
		goto err;
	CXD2817_SET_STATE(CXD2817_SHUTDOWN);
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2817_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct cxd2817_dev *cxd2817 = fe->demodulator_priv;
	return cxd2817_wr_mask(cxd2817, 0xdb, enable ? 1 : 0, 0x1);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
static int cxd2817_read_status(struct dvb_frontend *fe, enum fe_status *status)
#else
static int cxd2817_read_status(struct dvb_frontend *fe, fe_status_t *status)
#endif
{
	struct cxd2817_dev * cxd2817 = fe->demodulator_priv;
	int ret;
	u8 data[2];

	*status = 0;

	switch (cxd2817->delsys) {
	case CXD2817_DVBT:
		ret = cxd2817_rd(cxd2817, 0x0010, &data[0]);
		if (ret)
			goto err;
		if ((data[0] & 0x07) >= 6) {
			*status = FE_HAS_SIGNAL		|
				  FE_HAS_CARRIER	|
				  FE_HAS_VITERBI	|
				  FE_HAS_SYNC;
		}
		ret = cxd2817_rd(cxd2817, 0x0073, &data[0]);
		if (ret)
			goto err;
		if (data[0] & 0x08) {
			*status = FE_HAS_SIGNAL		|
				  FE_HAS_CARRIER	|
				  FE_HAS_VITERBI	|
				  FE_HAS_SYNC		|
				  FE_HAS_LOCK;
		}
		break;
	case CXD2817_DVBC:
		ret = cxd2817_rd(cxd2817, 0x10088, &data[0]);
		if (ret)
			goto err;
		ret = cxd2817_rd(cxd2817, 0x10073, &data[1]);
		if (ret)
			goto err;
		if (((data[0] >> 0) & 0x01) == 1) {
			*status = FE_HAS_SIGNAL		|
				  FE_HAS_CARRIER	|
				  FE_HAS_VITERBI	|
				  FE_HAS_SYNC;

			if (((data[1] >> 3) & 0x01) == 1) {
				*status = FE_HAS_SIGNAL		|
					  FE_HAS_CARRIER	|
					  FE_HAS_VITERBI	|
					  FE_HAS_SYNC		|
					  FE_HAS_LOCK;
			}
		}
		break;
	case CXD2817_NONE:
	default:
		ret = -EINVAL;
		goto err;
	}
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2817_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct cxd2817_dev *cxd2817 = fe->demodulator_priv;

	int ret;
	u16 tmp;
	u8 buf[2];

	*strength = 0;

	ret = cxd2817_rd_regs(cxd2817, 0x0026, buf, 2);
	if (ret)
		goto err;

	tmp = (buf[0] & 0x0f) << 8 | buf[1];
	tmp = ~tmp & 0x0fff;

	*strength = tmp * 0xffff / 0x0fff;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2817_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct cxd2817_dev *cxd2817 = fe->demodulator_priv;
	enum cxd2817_qam_const qam = CXD2817_QAM16;
	int ret;
	u8 data[2];
	u16 tmp;

	if (cxd2817->state != CXD2817_ACTIVE) {
		ret = -EINVAL;
		goto err;
	}
	*snr = 0;

	switch (cxd2817->delsys) {
	case CXD2817_DVBT:
		ret = cxd2817_wr(cxd2817, 0x00001, 0x01);
		if (ret)
			goto errt;
		ret = cxd2817_rd_regs(cxd2817, 0x00028, data, 2);
		if (ret)
			goto errt;
errt:
		ret = cxd2817_wr(cxd2817, 0x00001, 0x00);
		if (ret)
			goto err;
		tmp = ((data[0] & 0x1f) << 8) | data[1];
		if (!tmp)
			goto err; /* log10 returns infinity */

		*snr = 10 * 10 * (s32)intlog10(tmp) ;
		*snr -= 9031 ;
		break;
	case CXD2817_DVBC:
		ret = cxd2817_wr(cxd2817, 0x10001, 0x01);
		if (ret)
			goto err;
		ret = cxd2817_rd(cxd2817, 0x10088, &data[0]);
		if (ret)
			goto err;
		if (!(data[0] & 0x03)) {
			cxd2817_wr(cxd2817, 0x10001, 0x00);
			ret = -EINVAL;
			goto out;
		}
		ret = cxd2817_rd(cxd2817, 0x10019, &data[0]);
		if (ret)
			goto errc;
		qam = (data[0] & 0x7);

		ret = cxd2817_rd(cxd2817, 0x1004d, &data[0]);
		if (ret)
			goto errc;
errc:
		ret = cxd2817_wr(cxd2817, 0x10001, 0x00);
		if (ret)
			goto err;

		switch (qam) {
		case CXD2817_QAM16:
		case CXD2817_QAM64:
		case CXD2817_QAM256:
			*snr = -95 * intlog2(data[0]) + 63017;
			break;
		case CXD2817_QAM32:
		case CXD2817_QAM128:
			*snr = (-875 * intlog2(data[0]) + 566735) / 10;
			break;
		default:
			ret = -EINVAL;
			goto out;
		}
		break;
	case CXD2817_NONE:
	default:
		ret = -EINVAL;
		goto out;
	}
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2817_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct cxd2817_dev *cxd2817 = fe->demodulator_priv;
	int ret;
	u8 data[3] = { 0 };

	*ber = 0;

	if (cxd2817->state != CXD2817_ACTIVE) {
		ret = -EINVAL;
		goto err;
	}
	switch (cxd2817->delsys) {
	case CXD2817_DVBT:
		if (cxd2817->berc_run) {
			ret = cxd2817_rd_regs(cxd2817, 0x00076, data, 3);
			if (ret)
				goto err;
			if (!(data[2] & 0x80)) {
				ret = -EAGAIN;
				goto out;
			}
			*ber = ((data[2] & 0x0f) << 16) | (data[1] << 8) | data[0];
		} else {
			cxd2817->berc_run = 1;
			ret = cxd2817_wr(cxd2817, 0x00079, 0x01);
			if (ret)
				goto err;
		}
		break;
	case CXD2817_DVBC:
		ret = cxd2817_wr(cxd2817, 0x10001, 0x01);
		if (ret)
			goto errc;
		ret = cxd2817_rd(cxd2817, 0x10088, &data[0]);
		if (ret)
			goto errc;
		if (!(data[0] & 0x03)) {
			ret = -EINVAL;
			goto out;
		}
		if (cxd2817->berc_run) {
			ret = cxd2817_rd_regs(cxd2817, 0x10072, data, 3);
			if (ret)
				goto errc;
errc:
			ret = cxd2817_wr(cxd2817, 0x10001, 0x00);
			if (ret)
				goto err;
			if (!(data[2] & 0x80)) {
				ret = -EAGAIN;
				goto out;
			}
			*ber = ((data[2] & 0x0F) << 16) | (data[1] << 8) | data[0];
		} else {
			cxd2817->berc_run = 1;
			ret = cxd2817_wr(cxd2817, 0x10079, 0x01);
			if (ret)
				goto err;
		}
		break;
	case CXD2817_NONE:
	default:
		ret = -EINVAL;
		goto out;
	}
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2817_read_ucblocks(struct dvb_frontend *fe, u32 *ucb)
{
	struct cxd2817_dev *cxd2817 = fe->demodulator_priv;
	int ret;
	u8 data[2];
	*ucb = 0;

	switch (cxd2817->delsys) {
	case CXD2817_DVBT:
		ret = cxd2817_wr(cxd2817, 0x00001, 0x01);
		if (ret)
			goto errt;
		ret = cxd2817_rd_regs(cxd2817, 0x000ea, data, 2);
		if (ret)
			goto errt;
errt:
		ret = cxd2817_wr(cxd2817, 0x00001, 0x00);
		if (ret)
			goto err;
		break;
	case CXD2817_DVBC:
		ret = cxd2817_wr(cxd2817, 0x10001, 0x01);
		if (ret)
			goto errc;
		ret = cxd2817_rd(cxd2817, 0x10088, &data[0]);
		if (ret)
			goto errc;
		if (!(data[0] & 0x03)) {
			ret = -EINVAL;
			goto out;
		}
		ret = cxd2817_rd_regs(cxd2817, 0x100ea, data, 2);
		if (ret)
			goto errc;
errc:
		ret = cxd2817_wr(cxd2817, 0x10001, 0x00);
		if (ret)
			goto err;
		break;
	case CXD2817_NONE:
	default:
		ret = -EINVAL;
		goto out;
	}
	*ucb = (data[0] & 0x0f) << 8 | data[1];
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static inline s32 cxd2817_comp(u32 val, u32 b_len)
{
	if (b_len == 0 || b_len >= 32)
		return (s32) val;

	if (val & (1 << (b_len - 1)))
		return (s32) (MASK_HI(32 - b_len) | val); /* - */
	else
		return (s32) (MASK_LO(b_len) & val); /* + */
}

static int cxd2817_caroffst(struct cxd2817_dev *cxd2817, s32 *offst)
{
	u8 data[4];
	int ret;
	u32 val;

	*offst = 0;

	switch (cxd2817->delsys) {
	case CXD2817_DVBT:
		ret = cxd2817_wr(cxd2817, 0x00001, 0x01);
		if (ret)
			goto errt;
		ret = cxd2817_rd_regs(cxd2817, 0x0004c, data, 4);
		if (ret)
			goto errt;
		val = (data[0] & 0x1f << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
		*offst = -cxd2817_comp(val, 29);

		switch (cxd2817->bw) {
		case 6000000:
			*offst /= 39147;
			break;
		case 7000000:
			*offst /= 33554;
			break;
		case 8000000:
			*offst /= 29360;
			break;
		default:
			dprintk(FE_ERROR, 1, "Invalid bw, bw=%d", cxd2817->bw);
			ret = -EINVAL;
			goto out;
		}
		ret = cxd2817_rd(cxd2817, 0x07c6, &data[0]);
		if (ret)
			goto errt;
		if (data[0] & 0x01)
			*offst *= -1;
errt:
		ret = cxd2817_wr(cxd2817, 0x00001, 0x00);
		if (ret)
			goto err;
		break;
	case CXD2817_DVBC:
		ret = cxd2817_wr(cxd2817, 0x10001, 0x01);
		if (ret)
			goto errc;
		ret = cxd2817_rd_regs(cxd2817, 0x10015, data, 2);
		if (ret)
			goto errc;
		val = ((data[0] & 0x3f) << 8) | data[1];
		*offst = cxd2817_comp(val, 14);

		ret = cxd2817_rd(cxd2817, 0x10019, &data[0]);
		if (ret)
			goto errc;
		if (!(data[0] & 0x80))
			*offst *= -1;
errc:
		ret = cxd2817_wr(cxd2817, 0x10001, 0x00);
		if (ret)
			goto err;
		break;
	case CXD2817_NONE:
	default:
		ret = -EINVAL;
		goto out;
	}
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}


static int cxd2817_get_frontend_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_CUSTOM;
}

static enum dvbfe_search cxd2817_search(struct dvb_frontend *fe)
{
	struct cxd2817_dev *cxd2817		= fe->demodulator_priv;
	struct dtv_frontend_properties *props	= &fe->dtv_property_cache;
	enum cxd2817_delsys delsys;
	int ret = 0, i;
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
	enum fe_status status = 0;
	#else
	fe_status_t status = 0;
	#endif
	s32 offst;
	u32 step;

	if (!((props->delivery_system == SYS_DVBC_ANNEX_A) ||
	     (props->delivery_system == SYS_DVBT))) {

		dprintk(FE_ERROR, 1, "Unsupported delivery system, delsys=%d",
			props->delivery_system);

		ret = -EINVAL;
		goto err;
	}

	if (props->delivery_system == SYS_DVBC_ANNEX_A)
		delsys = CXD2817_DVBC;
	else if (props->delivery_system == SYS_DVBT)
		delsys = CXD2817_DVBT;
	else
		delsys = CXD2817_NONE;

	dprintk(FE_DEBUG, 1, "CXD2817 Delsys:%d", delsys);
	if (fe->ops.tuner_ops.set_params) {
		ret = fe->ops.tuner_ops.set_params(fe);
		if (ret)
			goto err;
	}
	ret = cxd2817_tune(cxd2817, delsys);
	if (ret)
		goto err;

	for (i = 0; i < 10; i++) {
		ret = cxd2817_read_status(fe, &status);
		if (ret)
			goto err;
		if (status & FE_HAS_LOCK)
			break;
		msleep(50);
	}
	if (status & FE_HAS_LOCK) {
		ret = cxd2817_caroffst(cxd2817, &offst);
		if (ret)
			goto err;
		dprintk(FE_DEBUG, 1, "Carrier OFFST=%d", offst);
		if (fe->ops.tuner_ops.info.frequency_step) {
			step = (fe->ops.tuner_ops.info.frequency_step + 500) / 1000;
			step = (step + 1) / 2;
			if (abs(offst) > step) {
				props->frequency += (offst * 1000);
				dprintk(FE_DEBUG, 1, "Offset > Tuner step, Fc=%d", props->frequency);
			}
		}
		ret = cxd2817_berm_ctl(cxd2817, CXD2817_BERC_START);
		if (ret)
			goto err;
		ret = DVBFE_ALGO_SEARCH_SUCCESS;
		goto out;
	}
	ret = DVBFE_ALGO_SEARCH_FAILED;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static struct dvb_frontend_ops cxd2817_ops = {
	.delsys = { SYS_DVBT, SYS_DVBC_ANNEX_A },
	.info = {
		.name			= "Sony CXD2817",
		.frequency_min		= 177000000,
		.frequency_max		= 858000000,
		.frequency_stepsize	= 166666,

		.caps = FE_CAN_FEC_1_2			|
			FE_CAN_FEC_2_3			|
			FE_CAN_FEC_3_4			|
			FE_CAN_FEC_5_6			|
			FE_CAN_FEC_7_8			|
			FE_CAN_FEC_AUTO			|
			FE_CAN_QPSK			|
			FE_CAN_QAM_16			|
			FE_CAN_QAM_32			|
			FE_CAN_QAM_64			|
			FE_CAN_QAM_128			|
			FE_CAN_QAM_256			|
			FE_CAN_QAM_AUTO			|
			FE_CAN_FEC_AUTO			|
			FE_CAN_HIERARCHY_AUTO		|
			FE_CAN_GUARD_INTERVAL_AUTO	|
			FE_CAN_TRANSMISSION_MODE_AUTO
	},

	.release		= cxd2817_release,
	.init 			= cxd2817_init,
	.sleep			= cxd2817_shutdown,
	.i2c_gate_ctrl 		= cxd2817_i2c_gate_ctrl,

	.get_frontend_algo	= cxd2817_get_frontend_algo,
	.search			= cxd2817_search,

	.read_status 		= cxd2817_read_status,
	.read_snr		= cxd2817_read_snr,
	.read_ber		= cxd2817_read_ber,
	.read_ucblocks		= cxd2817_read_ucblocks,
	.read_signal_strength	= cxd2817_read_signal_strength,
};

#define CXD2817_ID		0x70


struct dvb_frontend *cxd2817_attach(const struct cxd2817_config *config,
				    struct i2c_adapter *i2c)
{
	struct cxd2817_dev *cxd2817;
	u8 id;
	int ret;

	cxd2817 = kzalloc(sizeof (struct cxd2817_dev), GFP_KERNEL);
	if (!cxd2817)
		goto error;

	cxd2817->verbose		= &verbose;
	cxd2817->cfg			= config;
	cxd2817->i2c			= i2c;
	cxd2817->fe.ops			= cxd2817_ops;
	cxd2817->fe.demodulator_priv	= cxd2817;

	ret = cxd2817_rd(cxd2817, 0xfd, &id);
	if (ret < 0) {
		dprintk(FE_ERROR, 1, "Read error, ret=%d", ret);
	}
	if (id == CXD2817_ID)
		dprintk(FE_ERROR, 1, "Found CXD2817, ID=0x%02x", id);

	dprintk(FE_ERROR, 1, "Attaching CXD2817 demodulator(ID=0x%x)", id);
	return &cxd2817->fe;
error:
	kfree(cxd2817);
	return NULL;
}
EXPORT_SYMBOL(cxd2817_attach);
MODULE_PARM_DESC(verbose, "Set Verbosity level");
MODULE_AUTHOR("Manu Abraham");
MODULE_DESCRIPTION("CXD2817 Multi-Std Broadcast frontend");
MODULE_LICENSE("GPL");
