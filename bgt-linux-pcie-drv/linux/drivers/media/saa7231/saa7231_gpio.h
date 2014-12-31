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

#ifndef __SAA7231_GPIO_H
#define __SAA7231_GPIO_H

enum saa7231_gpio {
	GPIO_0		= (1 << 0),
	GPIO_1		= (1 << 1),
	GPIO_2		= (1 << 2),
	GPIO_3		= (1 << 3),
	GPIO_4		= (1 << 4),
	GPIO_5		= (1 << 5),
	GPIO_6		= (1 << 6),
	GPIO_7		= (1 << 7),
};

#define GPIO_SET_OUT(__gpios)		SAA7231_WR(__gpios, SAA7231_BAR0, GPIO, FB0_MODE1_SET)


extern int saa7231_gpio_set(struct saa7231_dev *saa7231, u8 gpio, u8 val);

#endif /* __SAA7231_GPIO_H */
