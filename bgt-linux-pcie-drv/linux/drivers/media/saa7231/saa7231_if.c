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

#include "saa7231_priv.h"

#include "saa7231_cgu_reg.h"
#include "saa7231_rgu_reg.h"
#include "saa7231_str_reg.h"
#include "saa7231_if_reg.h"
#include "saa7231_mod.h"
#include "saa7231_if.h"

int saa7231_if_init(struct saa7231_dev *saa7231)
{
	u32 i;
	dprintk(SAA7231_DEBUG, 1, "Initializing IF ..");

	for (i = 0; i < STREAM_PORT_TSOIF_PC; i++)
		SAA7231_WR(i, SAA7231_BAR0, IF_REGS, SEL_CHAN_DMA(i));

	return 0;
}
EXPORT_SYMBOL_GPL(saa7231_if_init);
