/*
	Sony CXD2861 Multistandard Tuner driver

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

#include "cxd2861.h"

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

enum cxd2861_delsys {
	CXD2861_8VSB,
	CXD2861_QAM,
	CXD2861_ISDBT_6,
	CXD2861_ISDBT_7,
	CXD2861_ISDBT_8,
	CXD2861_DVBT_5,
	CXD2861_DVBT_6,
	CXD2861_DVBT_7,
	CXD2861_DVBT_8,
	CXD2861_DVBT2_1_7,
	CXD2861_DVBT2_5,
	CXD2861_DVBT2_6,
	CXD2861_DVBT2_7,
	CXD2861_DVBT2_8,
	CXD2861_DVBC,
	CXD2861_DVBC2_6,
	CXD2861_DVBC2_8,
	CXD2861_DTMB,
};

static const char *namestr[] = {
	"CXD2861_8VSB",
	"CXD2861_QAM",
	"CXD2861_ISDBT_6",
	"CXD2861_ISDBT_7",
	"CXD2861_ISDBT_8",
	"CXD2861_DVBT_5",
	"CXD2861_DVBT_6",
	"CXD2861_DVBT_7",
	"CXD2861_DVBT_8",
	"CXD2861_DVBT2_1_7",
	"CXD2861_DVBT2_5",
	"CXD2861_DVBT2_6",
	"CXD2861_DVBT2_7",
	"CXD2861_DVBT2_8",
	"CXD2861_DVBC",
	"CXD2861_DVBC2_6",
	"CXD2861_DVBC2_8",
	"CXD2861_DTMB",
};

#define AUTO			0xff
#define OFFSET(__ofs)		((u8)(__ofs) & 0x1F)

#define BW_1_7			0x03
#define BW_6			0x00
#define BW_7			0x01
#define BW_8			0x02

static struct cxd2861_coe {
	enum cxd2861_delsys delsys;
	u8 if_out_sel;
	u8 agc_sel;
	u8 mix_oll;
	u8 rf_gain;
	u8 if_bpf_gc;
	u8 fif_offst;
	u8 bw_offst;
	u8 bw;
	u8 rf_oldet;
	u8 if_bpf_f0;
} coe[] = {
	{
		.delsys		= CXD2861_8VSB,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(-6),
		.bw_offst	= OFFSET(-6),
		.bw		= BW_6,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_QAM,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(-6),
		.bw_offst	= OFFSET(-6),
		.bw		= BW_6,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_ISDBT_6,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(-3),
		.bw_offst	= OFFSET(-0),
		.bw		= BW_6,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_ISDBT_7,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(-7),
		.bw_offst	= OFFSET(-5),
		.bw		= BW_7,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_ISDBT_8,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(-5),
		.bw_offst	= OFFSET(-3),
		.bw		= BW_8,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DVBT_5,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(-2),
		.bw_offst	= OFFSET(-4),
		.bw		= BW_6,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DVBT_6,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(-2),
		.bw_offst	= OFFSET(-4),
		.bw		= BW_6,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DVBT_7,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(-3),
		.bw_offst	= OFFSET(0),
		.bw		= BW_7,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DVBT_8,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(-3),
		.bw_offst	= OFFSET(0),
		.bw		= BW_8,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DVBT2_1_7,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(-5),
		.bw_offst	= OFFSET(0),
		.bw		= BW_1_7,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DVBT2_5,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(0),
		.bw_offst	= OFFSET(0),
		.bw		= BW_6,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DVBT2_6,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(0),
		.bw_offst	= OFFSET(0),
		.bw		= BW_6,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DVBT2_7,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(-6),
		.bw_offst	= OFFSET(-2),
		.bw		= BW_7,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DVBT2_8,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x06,
		.fif_offst	= OFFSET(-6),
		.bw_offst	= OFFSET(-3),
		.bw		= BW_8,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DVBC,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x02,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x03,
		.fif_offst	= OFFSET(-1),
		.bw_offst	= OFFSET(3),
		.bw		= BW_8,
		.rf_oldet	= 0x09,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DVBC2_6,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x01,
		.fif_offst	= OFFSET(-6),
		.bw_offst	= OFFSET(-4),
		.bw		= BW_6,
		.rf_oldet	= 0x09,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DVBC2_8,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x01,
		.fif_offst	= OFFSET(-2),
		.bw_offst	= OFFSET(2),
		.bw		= BW_8,
		.rf_oldet	= 0x09,
		.if_bpf_f0	= 0x00
	}, {
		.delsys		= CXD2861_DTMB,
		.if_out_sel	= AUTO,
		.agc_sel	= AUTO,
		.mix_oll	= 0x03,
		.rf_gain	= AUTO,
		.if_bpf_gc	= 0x02,
		.fif_offst	= OFFSET(2),
		.bw_offst	= OFFSET(3),
		.bw		= BW_8,
		.rf_oldet	= 0x0B,
		.if_bpf_f0	= 0x00
	}
};

enum cxd2861_agc {
	CXD2861_AGC2_ATV,
	CXD2861_AGC2_DTV
};

enum cxd2861_if {
	CXD2861_IF2_ATV,
	CXD2861_IF2_DTV
};

struct cxd2861_dev {
	struct dvb_frontend		*fe;
	struct i2c_adapter		*i2c;
	enum cxd2861_agc		agc2;
	enum cxd2861_if			if2;
	struct cxd2861_coe		*coe;
	const struct cxd2861_cfg	*cfg;
	u32				*verbose;
};

static int cxd2861_rd_regs(struct cxd2861_dev *cxd2861, u8 reg, u8 *data, int count)
{
	int ret;
	const struct cxd2861_cfg *config	= cxd2861->cfg;
	struct dvb_frontend *fe			= cxd2861->fe;
	struct i2c_msg msg[]			= {
		{ .addr = config->address, .flags = 0, 	   .buf = &reg, .len = 1 },
		{ .addr = config->address, .flags = I2C_M_RD, .buf = data, .len = count }
	};

	BUG_ON(count > 0x7f);
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);

	ret = i2c_transfer(cxd2861->i2c, msg, 2);
	if (ret != 2) {
		dprintk(FE_ERROR, 1, "I/O Error");
		ret = -EREMOTEIO;
	} else {
		ret = 0;
	}
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);

	return ret;
}

static int cxd2861_wr_regs(struct cxd2861_dev *cxd2861, u8 reg, u8 *data, u8 len)
{
	int i, ret;
	const struct cxd2861_cfg *config	= cxd2861->cfg;
	struct dvb_frontend *fe			= cxd2861->fe;
	u8 buf[0x7f];
	struct i2c_msg msg = { .addr = config->address, .flags = 0, .buf = buf, .len = len + 1 };

	BUG_ON(len > 0x7f);
	BUG_ON(reg > 0x7f);
	BUG_ON(reg + len > 0x7f);

	buf[0] = reg;
	memcpy(&buf[1], data, len);
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);

	for (i = 0; i < len; i++)
		dprintk(FE_DEBUG, 1, "Reg(0x%02x) = 0x%x", reg + i, data[i]);

	ret = i2c_transfer(cxd2861->i2c, &msg, 1);
	if (ret != 1) {
		dprintk(FE_ERROR, 1, "I/O Error");
		ret = -EREMOTEIO;
	} else {
		ret = 0;
	}
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);

	return ret;
}

static int cxd2861_wr(struct cxd2861_dev *cxd2861, u8 reg, u8 data)
{
	return cxd2861_wr_regs(cxd2861, reg, &data, 1);
}

static int cxd2861_rd(struct cxd2861_dev *cxd2861, u8 reg, u8 *data)
{
	return cxd2861_rd_regs(cxd2861, reg, data, 1);
}

static int cxd2861_wr_mask(struct cxd2861_dev *cxd2861, u32 reg, u8 val, u8 mask)
{
	int ret;
	u8 tmp;

	if (mask != 0xff) {
		ret = cxd2861_rd(cxd2861, reg, &tmp);
		if (ret)
			return ret;

		val &= mask;
		tmp &= ~mask;
		val |= tmp;
	}
	return cxd2861_wr(cxd2861, reg, val);
}

static int cxd2861_init(struct dvb_frontend *fe)
{
	struct cxd2861_dev *cxd2861	 = fe->tuner_priv;
	const struct cxd2861_cfg *config = cxd2861->cfg;
	int ret;
	u8 data[4];

	dprintk(FE_DEBUG, 1, "Initializing ..");
	if (config->f_xtal == 41) {
		ret = cxd2861_wr(cxd2861, 0x44, 0x07);
		if (ret)
			goto err;
	}

	data[0] = (u8)(config->f_xtal);
	data[1] = 0x06;
	data[2] = 0xC4;
	data[3] = 0x40;
	ret = cxd2861_wr_regs(cxd2861, 0x01, data, 4);
	if (ret)
		goto err;

	data[0] = 0x10;
	data[1] = 0x3f;
	data[2] = 0x25;
	ret = cxd2861_wr_regs(cxd2861, 0x22, data, 3);
	if (ret)
		goto err;
	ret = cxd2861_wr(cxd2861, 0x28, 0x1E);
	if (ret)
		goto err;
	ret = cxd2861_wr(cxd2861, 0x59, 0x04);
	if (ret)
		goto err;
	msleep(80);

	ret = cxd2861_rd_regs(cxd2861, 0x1A, data, 2);
	if (data[0] != 0x00) {
		ret = -EIO;
		goto err;
	}
#ifndef CXD2861_NVMERR
	if ((data[1] & 0x3F) != 0x00) {
		ret = -EIO;
		goto err;
	}
#endif
	if (!config->ext_osc) {
		ret = cxd2861_wr(cxd2861, 0x4C, 0x01);
		if (ret)
			goto err;
	}
	if (config->ext_osc)
		data[0] = 0x00;
	else
		data[0] = 0x04;

	ret = cxd2861_wr(cxd2861, 0x07, data[0]);
	if (ret)
		goto err;

	ret = cxd2861_wr(cxd2861, 0x04, 0x00);
	if (ret)
		goto err;

	ret = cxd2861_wr(cxd2861, 0x03, 0xC0);
	if (ret)
		goto err;

	data[0] = 0x00;
	data[1] = 0x04;
	ret = cxd2861_wr_regs(cxd2861, 0x14, data, 2);
	if (ret)
		goto err;
	ret = cxd2861_wr(cxd2861, 0x50, 0x01);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2861_sleep(struct dvb_frontend *fe)
{
	struct cxd2861_dev *cxd2861 = fe->tuner_priv;
	int ret;

	u8 data[2] = { 0x00, 0x04 };

	ret = cxd2861_wr_regs(cxd2861, 0x14, data, sizeof(data));
	if (ret)
		goto err;
	ret = cxd2861_wr(cxd2861, 0x50, 0x01);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2861_tune(struct cxd2861_dev *cxd2861, u32 frequency, enum cxd2861_delsys delsys, u8 vco_cal)
{
	const struct cxd2861_cfg *config = cxd2861->cfg;
	int ret;
	u8 data[10];

	frequency = ((frequency + 25 / 2) / 25) * 25;
	frequency /= 1000;

	data[0] = 0xfb;
	data[1] = 0x0f;
	ret = cxd2861_wr_regs(cxd2861, 0x14, data, 2);
	if (ret)
		goto err;
	ret = cxd2861_wr(cxd2861, 0x50, 0x00);
	if (ret)
		goto err;

	data[0] = 0x00;
	dprintk(FE_DEBUG, 1, "Frequency:%d (kHz)", frequency);
	dprintk(FE_DEBUG, 1, "Loading Coeffecient tables for %s delivery", namestr[delsys]);
	cxd2861->coe = &coe[delsys];
	if (coe[delsys].agc_sel == AUTO) {
		if (cxd2861->agc2 == CXD2861_AGC2_DTV)
			data[0] |= 0x08;
	} else {
		data[0] |= (u8)((coe[delsys].agc_sel & 0x03) << 3);
	}

	if (coe[delsys].if_out_sel == AUTO) {
		if (cxd2861->if2 == CXD2861_IF2_DTV)
			data[0] |= 0x04;
	} else {
		data[0] |= (u8)((coe[delsys].if_out_sel & 0x01) << 2);
	}
	ret = cxd2861_wr_mask(cxd2861, 0x05, data[0], 0x1c);
	if (ret)
		goto err;

	if (delsys == CXD2861_DVBC) {
		if (frequency > 500000)
			data[0] = (u8)((config->f_xtal == 41) ? 40 : (config->f_xtal));
		else
			data[0] = (u8)((config->f_xtal == 41) ? 82 : (config->f_xtal * 2));
	} else {
		if (frequency > 500000)
			data[0] = (u8)(config->f_xtal / 8);
		else
			data[0] = (u8)(config->f_xtal / 4);
	}
	if (config->ext_osc)
		data[1] = 0x00;
	else
		data[1] = 0x04;

	if (delsys == CXD2861_DVBC) {
		data[2] = 18;
		data[3] = 120;
		data[4] = 20;
	} else {
		data[2] = 48;
		data[3] = 10;
		data[4] = 30;
	}

	if (delsys == CXD2861_DVBC) {
		if (frequency > 500000)
			data[5] = 0x08;
		else
			data[5] = 0x0C;
	} else {
		if (frequency > 500000)
			data[5] = 0x30;
		else
			data[5] = 0x38;
	}
	data[6] = coe[delsys].mix_oll;

	if (coe[delsys].rf_gain == AUTO) {
		ret = cxd2861_wr(cxd2861, 0x4E, 0x01);
		data[7] = 0x00;
	} else {
		ret = cxd2861_wr(cxd2861, 0x4E, 0x00);
		data[7] = coe[delsys].rf_gain;
	}

	data[8] = (u8)((coe[delsys].fif_offst << 3) | (coe[delsys].if_bpf_gc & 0x07));

	data[9] = coe[delsys].bw_offst;
	ret = cxd2861_wr_regs(cxd2861, 0x06, data, 10);
	if (ret)
		goto err;

	if (delsys == CXD2861_DVBC) {
		data[0] = 0x0F;
		data[1] = 0x00;
		data[2] = 0x01;
	} else {
		data[0] = 0x0F;
		data[1] = 0x00;
		data[2] = 0x03;
	}
	ret = cxd2861_wr_regs(cxd2861, 0x45, data, 3);
	if (ret)
		goto err;

	data[0] = coe[delsys].rf_oldet;

	data[1] = coe[delsys].if_bpf_f0;
	ret = cxd2861_wr_regs(cxd2861, 0x49, data, 2);
	if (ret)
		goto err;

	if (vco_cal)
		data[0] = 0x90;
	else
		data[0] = 0x10;
	ret = cxd2861_wr_mask(cxd2861, 0x0C, data[0], 0xB0);

	data[0] = 0xc4;
	data[1] = 0x40;
	ret = cxd2861_wr_regs(cxd2861, 0x03, data, 2);
	if (ret)
		goto err;

	data[0] = (u8)(frequency & 0xFF);
	data[1] = (u8)((frequency >> 8) & 0xFF);
	data[2] = (u8)((frequency >> 16) & 0x0F);
	data[2] |= (u8)(coe[delsys].bw << 4);

	if (vco_cal)
		data[3] = 0xFF;
	else
		data[3] = 0x8F;

	data[4] = 0xFF;
	ret = cxd2861_wr_regs(cxd2861, 0x10, data, 5);
	if (ret)
		goto err;
	ret = cxd2861_wr(cxd2861, 0x04, 0x00);
	if (ret)
		goto err;
	ret = cxd2861_wr(cxd2861, 0x03, 0xC0);
	if (ret)
		goto err;
	ret = cxd2861_wr_mask(cxd2861, 0x0C, 0x00, 0x30);
	if (ret)
		goto err;
	dprintk(FE_DEBUG, 1, "Done!");
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

static int cxd2861_set_params(struct dvb_frontend *fe)
{
	struct cxd2861_dev *cxd2861 = fe->tuner_priv;
	struct dtv_frontend_properties *props	= &fe->dtv_property_cache;
	enum cxd2861_delsys delsys = SYS_UNDEFINED;
	int ret;

	switch (props->delivery_system) {
	case SYS_DVBC_ANNEX_AC:
		delsys = CXD2861_DVBC;
		break;
	case SYS_DVBC_ANNEX_B:
		delsys = CXD2861_QAM;
		break;
	case SYS_DVBT:
		switch (props->bandwidth_hz) {
		case 5000000:
			delsys = CXD2861_DVBT_5;
			break;
		case 6000000:
			delsys = CXD2861_DVBT_6;
			break;
		case 7000000:
			delsys = CXD2861_DVBT_7;
			break;
		case 8000000:
			delsys = CXD2861_DVBT_8;
			break;
		}
		break;
	case SYS_ISDBT:
		switch (props->bandwidth_hz) {
		case 6000000:
			delsys = CXD2861_ISDBT_6;
			break;
		case 7000000:
			delsys = CXD2861_ISDBT_7;
			break;
		case 8000000:
			delsys = CXD2861_ISDBT_8;
			break;
		}
		break;
	case SYS_ATSC:
		delsys = CXD2861_8VSB;
		break;
	case SYS_DMBTH:
		delsys = CXD2861_DTMB;
		break;
	case SYS_DVBT2:
		switch (props->bandwidth_hz) {
		case 1700000:
			delsys = CXD2861_DVBT2_1_7;
			break;
		case 5000000:
			delsys = CXD2861_DVBT2_5;
			break;
		case 6000000:
			delsys = CXD2861_DVBT2_6;
			break;
		case 7000000:
			delsys = CXD2861_DVBT2_7;
			break;
		case 8000000:
			delsys = CXD2861_DVBT2_8;
			break;
		}
		break;
	default:
		dprintk(FE_ERROR, 1, "Invalid delivery_system, delsys=%d", props->delivery_system);
		ret = -EINVAL;
		goto out;
	}
	dprintk(FE_DEBUG, 1, "Tune requested, f:%d", props->frequency);
	ret = cxd2861_tune(cxd2861, props->frequency, delsys, 1);
	if (ret)
		goto err;
	goto out;
err:
	dprintk(FE_ERROR, 1, "I/O error, ret=%d", ret);
out:
	return ret;
}

#define BW_1_7			0x03
#define BW_6			0x00
#define BW_7			0x01
#define BW_8			0x02

static int cxd2861_get_ifreq(struct dvb_frontend *fe, u32 *frequency)
{
	struct cxd2861_dev *cxd2861	= fe->tuner_priv;
	struct cxd2861_coe *coe		= cxd2861->coe;

	switch (coe->delsys) {
	case CXD2861_8VSB:
	case CXD2861_QAM:
		*frequency = 3700000;
		break;
	case CXD2861_ISDBT_6:
		*frequency = 3550000;
		break;
	case CXD2861_ISDBT_7:
		*frequency = 4150000;
		break;
	case CXD2861_ISDBT_8:
		*frequency = 4750000;
		break;
	case CXD2861_DVBT2_1_7:
		*frequency = 3500000;
		break;
	case CXD2861_DVBT_5:
	case CXD2861_DVBT_6:
	case CXD2861_DVBT2_5:
	case CXD2861_DVBT2_6:
		*frequency = 3600000;
		break;
	case CXD2861_DVBT_7:
	case CXD2861_DVBT2_7:
		*frequency = 4200000;
		break;
	case CXD2861_DVBT_8:
	case CXD2861_DVBT2_8:
		*frequency = 4800000;
		break;
	case CXD2861_DVBC:
		*frequency = 4900000;
		break;
	case CXD2861_DVBC2_6:
		*frequency = 3700000;
		break;
	case CXD2861_DVBC2_8:
		*frequency = 4900000;
		break;
	case CXD2861_DTMB:
		*frequency = 5100000;
		break;
	}
	return 0;
}

static int cxd2861_release(struct dvb_frontend *fe)
{
	struct cxd2861_dev *cxd2861 = fe->tuner_priv;

	BUG_ON(!cxd2861);
	fe->tuner_priv = NULL;
	kfree(cxd2861);
	return 0;
}

static struct dvb_tuner_ops cxd2861_ops = {

	.info = {
		.name		= "CXD2861 Silicon Tuner",
		.frequency_min  =  42000000,
		.frequency_max  = 870000000,
		.frequency_step	= 50000,
	},

	.init			= cxd2861_init,
	.sleep			= cxd2861_sleep,
	.set_params		= cxd2861_set_params,
	.get_if_frequency	= cxd2861_get_ifreq,
	.release		= cxd2861_release
};

struct dvb_frontend *cxd2861_attach(struct dvb_frontend *fe,
				    const struct cxd2861_cfg *config,
				    struct i2c_adapter *i2c)
{
	struct cxd2861_dev *cxd2861;
	u8 id;
	int ret;

	cxd2861 = kzalloc(sizeof (struct cxd2861_dev), GFP_KERNEL);
	if (!cxd2861)
		goto error;

	cxd2861->verbose	= &verbose;
	cxd2861->cfg		= config;
	cxd2861->i2c		= i2c;
	cxd2861->fe		= fe;
	fe->tuner_priv		= cxd2861;
	fe->ops.tuner_ops	= cxd2861_ops;

	ret = cxd2861_rd(cxd2861, 0x7f, &id);
	if (ret < 0)
		dprintk(FE_ERROR, 1, "Read error, ret=%d", ret);

	if (id == 0x71) {
		dprintk(FE_ERROR, 1, "Attaching CXD2861 tuner(ID=0x%x)", id);
		return cxd2861->fe;
	}
error:
	kfree(cxd2861);
	return NULL;
}
EXPORT_SYMBOL(cxd2861_attach);
MODULE_PARM_DESC(verbose, "Set Verbosity level");
MODULE_AUTHOR("Manu Abraham");
MODULE_DESCRIPTION("CXD2861 Multi-Std Broadcast frontend");
MODULE_LICENSE("GPL");
