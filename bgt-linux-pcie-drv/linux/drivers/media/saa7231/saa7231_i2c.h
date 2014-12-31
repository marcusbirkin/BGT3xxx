/*
 *	SAA7231xx PCI/PCI Express bridge driver
 *
 *	Copyright (C) Manu Abraham <abraham.manu@gmail.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __SAA7231_I2C_H
#define __SAA7231_I2C_H

struct saa7231_i2c {
	struct i2c_adapter		i2c_adapter;
	struct mutex			i2c_lock;
	struct saa7231_dev		*saa7231;
	u8				i2c_dev;

	enum saa7231_i2c_rate		i2c_rate;
	wait_queue_head_t		i2c_wq;
	u32				int_stat;
};

extern int saa7231_i2c_init(struct saa7231_dev *saa7231);
extern void saa7231_i2c_exit(struct saa7231_dev *saa7231);

#endif /* __SAA7231_H */
