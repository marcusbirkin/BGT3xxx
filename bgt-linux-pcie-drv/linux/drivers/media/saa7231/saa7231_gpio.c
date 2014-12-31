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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <linux/spinlock.h>

#include "saa7231_priv.h"
#include "saa7231_gpio.h"
#include "saa7231_gpio_reg.h"
#include "saa7231_mod.h"

int saa7231_gpio_set(struct saa7231_dev *saa7231, u8 bits, u8 val)
{
	if (val)
		SAA7231_WR(bits, SAA7231_BAR0, GPIO, FB0_MODE0_SET);
	else
		SAA7231_WR(bits, SAA7231_BAR0, GPIO, FB0_MODE0_RESET);

	return 0;
}
EXPORT_SYMBOL(saa7231_gpio_set);
