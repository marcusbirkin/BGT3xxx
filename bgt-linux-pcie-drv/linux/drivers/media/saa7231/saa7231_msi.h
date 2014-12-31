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

#ifndef __SAA7231_MSI_H
#define __SAA7231_MSI_H

#define SAA7231_MSI_VECTORS		128

#define SAA7231_MSI_REGS		128
#define SAA7231_MSI_BITS		128
#define SAA7231_REG_WIDTH		32
#define SAA7231_MSI_LOOPS		(SAA7231_MSI_BITS / SAA7231_REG_WIDTH)
#define DEFAULT_SAA7231_INTA_POLARITY	0
#define MSI_MODULE_ID_POR		0xa1090000

enum saa7231_edge {
	SAA7231_EDGE_RISING	= 1,
	SAA7231_EDGE_FALLING	= 2,
	SAA7231_EDGE_ANY	= 3
};

struct saa7231_irq_entry {
	int			vector;
	enum saa7231_edge	polarity;
	char			desc[32];
	int (*handler)		(struct saa7231_dev *saa7231, int vector);
};

struct saa7231_msix_entry {
	int			vector;
	enum saa7231_edge	polarity;
	char			desc[32];
	irqreturn_t (*handler)(int irq, void *dev_id);
};

struct saa7231_stat {
	u32 stat_0;
	u32 stat_1;
	u32 stat_2;
	u32 stat_3;
};

extern int saa7231_msi_init(struct saa7231_dev *saa7231);
extern void saa7231_msi_exit(struct saa7231_dev *saa7231);

extern int saa7231_add_irqevent(struct saa7231_dev *saa7231,
				int vector,
				enum saa7231_edge edge,
				int (*handler)(struct saa7231_dev *saa7231, int vector),
				char *desc);

extern int saa7231_remove_irqevent(struct saa7231_dev *saa7231, int vector);

extern int saa7231_msi_event(struct saa7231_dev *saa7231, struct saa7231_stat stat);

extern int saa7231_add_msivector(struct saa7231_dev *saa7231,
				 int vector,
				 enum saa7231_edge edge,
				 irqreturn_t (*handler)(int irq, void *dev_id),
				 char *desc);

extern int saa7231_remove_msivector(struct saa7231_dev *saa7231, int vector);
#endif /* __SAA7231_MSI_H */
