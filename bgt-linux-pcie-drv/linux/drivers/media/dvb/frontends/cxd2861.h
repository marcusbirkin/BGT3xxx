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

#ifndef __CXD2861_H
#define __CXD2861_H

struct cxd2861_cfg {
	u8	address;

	u32	f_xtal;
	bool	ext_osc;

	bool	ts_serial;
	bool	ts_ser_msb;
	bool	tsval_hi;
	bool	tssyn_hi;
	bool	tsclk_inv;
	bool	tsclk_cont;
	bool	ts_smooth;
	bool	ts_auto;

};

#if defined(CONFIG_DVB_CXD2861) || (defined(CONFIG_DVB_CXD2861_MODULE) && defined(MODULE))

extern struct dvb_frontend *cxd2861_attach(struct dvb_frontend *fe,
					   const struct cxd2861_cfg *config,
					   struct i2c_adapter *i2c);
#else

static inline struct dvb_frontend *cxd2861_attach(struct dvb_frontend *fe,
						  const struct cxd2861_cfg *config,
						  struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif /* CONFIG_DVB_CXD2861 */

#endif /* __CXD2861_H */
