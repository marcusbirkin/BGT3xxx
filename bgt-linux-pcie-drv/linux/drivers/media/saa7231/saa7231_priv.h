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

#ifndef __SAA7231_PRIV_H
#define __SAA7231_PRIV_H

#define SAA7231_ERROR		0
#define SAA7231_NOTICE		1
#define SAA7231_INFO		2
#define SAA7231_DEBUG		3

#define SAA7231_DEV		(saa7231)->num
#define SAA7231_VERBOSE		(saa7231)->verbose

#define dprintk(__x, __y, __fmt, __arg...) do {								\
	if (__y) {											\
		if	((SAA7231_VERBOSE > SAA7231_ERROR) && (SAA7231_VERBOSE > __x))			\
			printk(KERN_ERR "%s (%d): " __fmt "\n" , __func__ , SAA7231_DEV , ##__arg);	\
		else if	((SAA7231_VERBOSE > SAA7231_NOTICE) && (SAA7231_VERBOSE > __x))			\
			printk(KERN_NOTICE "%s (%d): " __fmt "\n" , __func__ , SAA7231_DEV , ##__arg);	\
		else if ((SAA7231_VERBOSE > SAA7231_INFO) && (SAA7231_VERBOSE > __x))			\
			printk(KERN_INFO "%s (%d): " __fmt "\n" , __func__ , SAA7231_DEV , ##__arg);	\
		else if ((SAA7231_VERBOSE > SAA7231_DEBUG))						\
			printk(KERN_DEBUG "%s (%d): " __fmt "\n" , __func__ , SAA7231_DEV , ##__arg);	\
	} else {											\
		if (SAA7231_VERBOSE > __x)								\
			printk(__fmt , ##__arg);							\
	}												\
} while(0)


#define BAR_0			0
#define BAR_2			2

#define NXP_SEMICONDUCTOR	0x1131
#define SAA7231_CHIP		0x7231

#define NXP_REFERENCE_BOARD	0x1131

#define MAKE_ENTRY(__subven, __subdev, __chip, __configptr) {	\
		.vendor		= NXP_SEMICONDUCTOR,		\
		.device		= SAA7231_CHIP,			\
		.subvendor	= (__subven),			\
		.subdevice	= (__subdev),			\
		.driver_data	= (unsigned long) (__configptr)	\
}

#define SAA7231_DEVID(__ver) (			\
	((__ver >> 8) > 6) ?			\
		(__ver >> 4):			\
		(__ver >> 4) - 1		\
)

#define SAA7231_TYPE(__ver)	chip_name[SAA7231_DEVID(__ver)]
#define SAA7231_CAPS(__ver)	chip_caps[SAA7231_DEVID(__ver)]


enum saa7231_type {
	/* LBGA 256 */
	SAA7231E	= 0x001,
	SAA7231AE	= 0x011,
	SAA7231BE	= 0x021,
	SAA7231CE	= 0x031,
	SAA7231HE	= 0x041,
	SAA7231JE	= 0x051,
	SAA7231KE	= 0x061,

	/* LFBGA 136 */
	SAA7231DE	= 0x08a,
	SAA7231FE	= 0x09a,
	SAA7231GE	= 0x10a,
	SAA7231ME	= 0x11a,
	SAA7231NE	= 0x12a
};

enum saa7231_bar {
	SAA7231_BAR0	= 0x00,
	SAA7231_BAR1	= 0x01,
};

enum saa7231_stream_port {
	STREAM_PORT_VS2D_AVIS		=  0,
	STREAM_PORT_VS2D_ITU		=  2,
	STREAM_PORT_DS2D_AVIS		=  4,
	STREAM_PORT_DS2D_ITU		=  5,
	STREAM_PORT_AS2D_LOCAL		=  6,
	STREAM_PORT_AS2D_EXTERN		=  7,
	STREAM_PORT_TS2D_DTV0		=  8,
	STREAM_PORT_TS2D_DTV1		=  9,
	STREAM_PORT_TS2D_EXTERN0	= 10,
	STREAM_PORT_TS2D_EXTERN1	= 11,
	STREAM_PORT_TS2D_CAM		= 12,
	STREAM_PORT_TSOIF_PC		= 13,
	STREAM_PORT_TSOIF_DCSN		= 14,
	STREAM_PORT_UNITS		= 15,
};

enum saa7231_tscfg {
	STREAM_ENABLE			= (1 << 0),
	STREAM_CLK_EDGE			= (1 << 1),
	STREAM_SYNC_EDGE		= (1 << 2),
	STREAM_VALID_EDGE		= (1 << 3),
	STREAM_INPUT_MODE		= (1 << 4),
	STREAM_SHIFT_DIR		= (1 << 5),
	STREAM_SYNC_DET			= (1 << 6),
};

struct saa7231_caps {
	enum saa7231_type	id;

	unsigned int		pci:1;
	unsigned int		pcie:1;
	unsigned int		ca:1;

	unsigned int		btsc:1;
	unsigned int		nicam:1;

	unsigned int		dvbs:1;
	unsigned int		dvbt:1;
}__attribute__((packed));


enum saa7231_cgu_rootclk {
	CLK_ROOT_27MHz,
	CLK_ROOT_54MHz,
	CLK_ROOT_CUSTOM
};

enum saa7231_i2c_rate {
	SAA7231_I2C_RATE_400 	= 1,
	SAA7231_I2C_RATE_100,
};

enum adapter_type {
	ADAPTER_INT		= 1,
	ADAPTER_EXT		= 2,
};

struct saa7231_dev;
struct saa7231_dvb;
struct saa7231_video;

struct card_desc {
	char *vendor;
	char *product;
	char *type;
};

struct saa7231_config {
	struct card_desc		*desc;

	unsigned int			a_tvc :1;
	unsigned int			v_cap :1;
	unsigned int			a_cap :1;

	enum saa7231_i2c_rate		i2c_rate;
	u32				xtal;

	enum saa7231_cgu_rootclk	root_clk;

	struct saa7231_input		*inputs;

	int				ext_dvb_adapters;
	enum saa7231_tscfg		ts0_cfg;
	enum saa7231_tscfg		ts1_cfg;
	u8				ts0_clk;
	u8				ts1_clk;

	int (*frontend_enable)(struct saa7231_dev *saa7231);
	int (*frontend_attach)(struct saa7231_dvb *dvb, int count);
	irqreturn_t (*irq_handler)(int irq, void *dev_id);

	u8				stream_ports;
};

struct saa7231_dev {

	struct list_head		devlist;

	struct saa7231_config		*config;
	struct pci_dev			*pdev;

	int				num;

	int				verbose;

	struct mutex			dev_lock;

	u8				version;
	char				name[10];
	char				ver[10];

	u8				msi_vectors_max;

	void __iomem			*mmio1;
	void __iomem			*mmio2;

	u32				offst_bar0;
	u32				offst_bar2;

	struct saa7231_caps		caps;

#define MODE_INTA	0
#define MODE_MSI	1
#define MODE_MSI_X	2
	u8				int_type;
	struct msix_entry		*msix_entries;
	struct saa7231_msix_entry	*saa7231_msix_handler;

	u8				handlers;

	struct saa7231_irq_entry	*event_handler;
	struct saa7231_i2c		*i2c;

	u32				i2c_rate;
	u32				I2C_DEV[4];

	struct saa7231_cgu		*cgu;
	struct saa7231_dvb		*dvb;
	u8				adapters;

	struct saa7231_video		*video;
	struct saa7231_audio		*audio;
};

#define SAA7231_BAR0	saa7231->mmio1
#define SAA7231_BAR2	saa7231->mmio2

#define SAA7231_REG(__offst, __addr)			(__offst + __addr)

#define SAA7231_WR(__data, __bar, __module, __addr)	(writel((__data), (__bar + (__module + __addr))))
#define SAA7231_RD(__bar, __module, __addr)		(readl(__bar + (__module + __addr)))

#define SAA7231_SETFIELD(mask, bitf, val)	\
	(mask = (mask & (~(((1 << WIDTH_##bitf) - 1) << OFFST_##bitf))) | (val << OFFST_##bitf))

#define SAA7231_GETFIELD(val, bitf)		\
	((val >> OFFST_##bitf) & ((1 << WIDTH_##bitf) - 1))

extern int saa7231_pci_init(struct saa7231_dev *saa7231);
extern void saa7231_pci_exit(struct saa7231_dev *saa7231);
extern int saa7231_device_add(struct saa7231_dev *saa7231);

#endif /* __SAA7231_PRIV_H */
