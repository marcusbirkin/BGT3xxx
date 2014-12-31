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
#include <linux/delay.h>
#include <linux/interrupt.h>

#include "saa7231_priv.h"
#include "saa7231_mod.h"
#include "saa7231_dcs_reg.h"
#include "saa7231_msi_reg.h"
#include "saa7231_msi.h"

#define SAA7231_MSI_VECTORS			128

int saa7231_msi_event(struct saa7231_dev *saa7231, struct saa7231_stat stat)
{
	int i, j;
	u32 enable, status;

	dprintk(SAA7231_DEBUG, 0, "MSI event ");
	for (i = 0; i < SAA7231_MSI_LOOPS; i++) {
		enable = SAA7231_RD(SAA7231_BAR0, MSI, MSI_INT_ENA(i));
		status = SAA7231_RD(SAA7231_BAR0, MSI, MSI_INT_STATUS(i));
		for (j = 0; j < SAA7231_REG_WIDTH; j++) {
			if ((enable & (1 << j)) & (status & (1 << j)))
				dprintk(SAA7231_DEBUG, 1, "MSI Event:%d", (i + 1) * j);
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(saa7231_msi_event);

void saa7231_msi_exit(struct saa7231_dev *saa7231)
{
	struct saa7231_msix_entry *msix_handler = saa7231->saa7231_msix_handler;
	struct saa7231_irq_entry *event_handler = saa7231->event_handler;

	BUG_ON(!msix_handler);
	BUG_ON(!event_handler);

	dprintk(SAA7231_DEBUG, 1, "Exiting with %d active events", saa7231->handlers);
	kfree(msix_handler);
	kfree(event_handler);
}
EXPORT_SYMBOL_GPL(saa7231_msi_exit);

int saa7231_msi_init(struct saa7231_dev *saa7231)
{
	u32 id, enable, status;
	int i, j;

	struct saa7231_msix_entry *msix_handlers = NULL;
	struct saa7231_irq_entry *event_handlers = NULL;

	msix_handlers = kzalloc((sizeof (struct saa7231_msix_entry) * 128), GFP_KERNEL);
	if (!msix_handlers) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Could not allocate MSI-X handlers");
		return -ENOMEM;
	}
	saa7231->saa7231_msix_handler = msix_handlers;

	event_handlers = kzalloc((sizeof (struct saa7231_irq_entry) * 128), GFP_KERNEL);
	if (!event_handlers) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Could not allocate IRQ event handlers");
		return -ENOMEM;
	}
	saa7231->event_handler = event_handlers;

	SAA7231_WR(0x01, SAA7231_BAR0, MSI, MSI_SW_RST);

	id = SAA7231_RD(SAA7231_BAR0, MSI, MSI_MODULE_ID);
	if ((id & ~0x00000F00) != MSI_MODULE_ID_POR) {
		dprintk(SAA7231_ERROR, 1, "MSI Module Version 0x%02x, unsupported", id);
		return -EINVAL;
	}

	SAA7231_WR(0x00, SAA7231_BAR0, MSI, MSI_DELAY_TIMER);
	SAA7231_WR(DEFAULT_SAA7231_INTA_POLARITY, SAA7231_BAR0, MSI, MSI_INTA_POLARITY);

	for (i = 0; i < SAA7231_MSI_REGS; i++)
		SAA7231_WR(0x01000000, SAA7231_BAR0, MSI, MSI_CONFIG(i));

	for (i = 0; i < SAA7231_MSI_LOOPS; i++) {
		enable = SAA7231_RD(SAA7231_BAR0, MSI, MSI_INT_ENA(i));
		status = SAA7231_RD(SAA7231_BAR0, MSI, MSI_INT_STATUS(i));
		if (enable)
			SAA7231_WR(enable, SAA7231_BAR0, MSI, MSI_INT_ENA_CLR(i));
		if (status)
			SAA7231_WR(status, SAA7231_BAR0, MSI, MSI_INT_STATUS_CLR(i));
	}

	msleep(5);

	for (i = 0; i < SAA7231_MSI_LOOPS; i++) {
		enable = SAA7231_RD(SAA7231_BAR0, MSI, MSI_INT_ENA(i));
		status = SAA7231_RD(SAA7231_BAR0, MSI, MSI_INT_STATUS(i));
		for (j = 0; j < SAA7231_REG_WIDTH; j++) {
			if ((enable & (1 << j)) & (status & (1 << j))) {
				dprintk(SAA7231_ERROR, 1, "MSI interrupt %d RESET failed", (i + 1) * j);
				return -1; /* RESET failed */
			}
		}
	}
#if 0
	/* enable DCSN interrupt handling */
	SAA7231_WR(3, SAA7231_BAR0, DCSN, DCSN_INT_CLR_STATUS);
	SAA7231_WR(0x100000, SAA7231_BAR0, MSI, MSI_INT_ENA_SET(1));
	SAA7231_WR(3, SAA7231_BAR0, DCSN, DCSN_INT_SET_ENABLE);
#endif
	saa7231->handlers = 0;
	return  0;
}
EXPORT_SYMBOL_GPL(saa7231_msi_init);


static void saa7231_vector_ctl(struct saa7231_dev *saa7231, int vector, u8 ctl)
{
	u32 reg, tmp = 1;

	tmp <<= (vector % 32);
	reg = SAA7231_RD(SAA7231_BAR0, MSI, MSI_INT_ENA((vector / 32)));

	if (ctl)
		SAA7231_WR(reg | tmp, SAA7231_BAR0, MSI, MSI_INT_ENA_SET((vector / 32)));
	else
		SAA7231_WR(reg | tmp, SAA7231_BAR0, MSI, MSI_INT_ENA_CLR((vector / 32)));
	return;
}

static int saa7231_setup_vector(struct saa7231_dev *saa7231,
				int vector,
				enum saa7231_edge edge)
{
	u32 config;

	BUG_ON(!saa7231);
	BUG_ON(vector > SAA7231_MSI_VECTORS);

	dprintk(SAA7231_DEBUG, 1, "Adding Vector:%d", vector);

	config = SAA7231_RD(SAA7231_BAR0, MSI, MSI_CONFIG(vector));
	config &= 0xfcffffff;

	switch (edge) {
	case SAA7231_EDGE_FALLING:
		config |= 0x02000000;
		break;
	case SAA7231_EDGE_ANY:
		config |= 0x03000000;
		break;
	case SAA7231_EDGE_RISING:
	default:
		config |= 0x01000000;
		break;
	}

	SAA7231_WR(config, SAA7231_BAR0, MSI, MSI_CONFIG(vector));

	dprintk(SAA7231_DEBUG, 1, "Enabling Vector:%d", vector);
	saa7231_vector_ctl(saa7231, vector, 1);
	return 0;
}


int saa7231_add_msivector(struct saa7231_dev *saa7231,
			  int vector,
			  enum saa7231_edge edge,
			  irqreturn_t (*handler)(int irq, void *dev_id),
			  char *desc)
{
	struct saa7231_msix_entry *msix = &saa7231->saa7231_msix_handler[vector];

	BUG_ON(!msix);

	saa7231_setup_vector(saa7231, vector, edge);
	strcpy(msix->desc, desc);
	msix->handler = handler;
	saa7231->handlers++;
	return 0;
}
EXPORT_SYMBOL_GPL(saa7231_add_msivector);

int saa7231_add_irqevent(struct saa7231_dev *saa7231,
			 int vector,
			 enum saa7231_edge edge,
			 int (*handler)(struct saa7231_dev *saa7231, int vector),
			 char *desc)
{
	struct saa7231_irq_entry *event = &saa7231->event_handler[vector];

	BUG_ON(!event);
	dprintk(SAA7231_DEBUG, 1, "Adding %s IRQ Event:%d ...", desc, vector);

	saa7231_setup_vector(saa7231, vector, edge);
	strcpy(event->desc, desc);
	event->handler = handler;
	event->vector = vector;
	saa7231->handlers++;
	dprintk(SAA7231_DEBUG, 1, "Succesfully added %s as Event handler:%d", event->desc, vector);
	return 0;
}
EXPORT_SYMBOL_GPL(saa7231_add_irqevent);

int saa7231_remove_irqevent(struct saa7231_dev *saa7231, int vector)
{
	struct saa7231_irq_entry *event = &saa7231->event_handler[vector];

	BUG_ON(!event);
	dprintk(SAA7231_DEBUG, 1, "Removing IRQ Event %d <%s> ..",
		event->vector,
		event->desc);

	if (event->handler) {
		event->vector = 0;
		event->handler = NULL;
		saa7231->handlers--;
		saa7231_vector_ctl(saa7231, vector, 0);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(saa7231_remove_irqevent);


int saa7231_remove_msivector(struct saa7231_dev *saa7231, int vector)
{
	struct saa7231_msix_entry *msix = &saa7231->saa7231_msix_handler[vector];
	int i;

	BUG_ON(!msix);

	dprintk(SAA7231_DEBUG, 1, "Removing MSI Vector:%d .......", vector);
	for (i = 0; i < 128; i++) {
		if (msix->vector == vector) {
			dprintk(SAA7231_DEBUG, 1, "MSI Vector %d <%s> removed",
				msix->vector,
				msix->desc);

			if (msix->handler) {
				msix->vector = 0;
				msix->handler = NULL;
				saa7231->handlers--;
				saa7231_vector_ctl(saa7231, vector, 0);
			}
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(saa7231_remove_msivector);
