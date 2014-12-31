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

#ifndef __A8290_H
#define __A8290_H

struct a8290_config {
	u8 address;
	u8 ext_mod;
	u8 i2c_txrx_ctl;
};


#if defined(CONFIG_DVB_A8290) || (defined(CONFIG_DVB_A8290_MODULE) && defined(MODULE))

extern struct dvb_frontend *a8290_attach(struct dvb_frontend *fe,
					 const struct a8290_config *config,
					 struct i2c_adapter *i2c);
#else

static inline struct dvb_frontend *a8290_attach(struct dvb_frontend *fe,
						const struct a8290_config *config,
						struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif /* CONFIG_DVB_A8290 */

#endif /* __A8290_H */
