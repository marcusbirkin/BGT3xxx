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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/mutex.h>

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>

#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/device.h>

#include <linux/dvb/video.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/osd.h>

#include "dvbdev.h"
#include "dvb_demux.h"
#include "dmxdev.h"
#include "dvb_frontend.h"
#include "dvb_net.h"
#include "dvb_ca_en50221.h"

#include <linux/videodev2.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>

#include "saa7231_mod.h"
#include "saa7231_priv.h"
#include "saa7231_dvb.h"
#include "saa7231_cgu.h"
#include "saa7231_cgu_reg.h"
#include "saa7231_if.h"
#include "saa7231_i2c.h"
#include "saa7231_msi_reg.h"
#include "saa7231_dcs_reg.h"
#include "saa7231_msi.h"
#include "saa7231_gpio_reg.h"
#include "saa7231_gpio.h"
//#include "saa7231_vs2dtl.h"
//#include "saa7231_vidops.h"
//#include "saa7231_audops.h"
//#include "saa7231_aico_reg.h"

//#include "saa7231_dvbs.h"
//#include "saa7231_dvbt.h"

#include "stv6110x.h"
#include "stv090x.h"
#include "lnbh24.h"
#include "tda10048.h"
#include "tda18271.h"
#include "s5h1411.h"
#include "tda18272.h"
#include "cxd2820r.h"
//#include "cxd2834.h"
#include "cxd2850.h"
#include "a8290.h"
#include "cxd2817.h"
#include "cxd2861.h"

static unsigned int verbose;
static unsigned int int_type;

module_param(verbose, int, 0644);
module_param(int_type, int, 0644);

MODULE_PARM_DESC(verbose, "verbose startup messages, default is 1 (yes)");
MODULE_PARM_DESC(int_type, "force Interrupt Handler type: 0=INT-A, 1=MSI, 2=MSI-X. default INT-A mode");

#define DRIVER_NAME				"SAA7231"
#define DRIVER_VER				"0.0.91"
#define MODULE_DBG				(((saa7231)->verbose == SAA7231_DEBUG) ? 1 : 0)

extern void saa7231_dump_write(struct saa7231_dev *saa7231);

static unsigned int num;

#define STV090x_CFGUPDATE(__config, __ctl) {				\
	(__config)->tuner_init		= (__ctl)->tuner_init;		\
	(__config)->tuner_set_mode	= (__ctl)->tuner_set_mode;	\
	(__config)->tuner_set_frequency	= (__ctl)->tuner_set_frequency;	\
	(__config)->tuner_get_frequency	= (__ctl)->tuner_get_frequency;	\
	(__config)->tuner_set_bandwidth	= (__ctl)->tuner_set_bandwidth;	\
	(__config)->tuner_get_bandwidth	= (__ctl)->tuner_get_bandwidth;	\
	(__config)->tuner_set_bbgain	= (__ctl)->tuner_set_bbgain;	\
	(__config)->tuner_get_bbgain	= (__ctl)->tuner_get_bbgain;	\
	(__config)->tuner_set_refclk	= (__ctl)->tuner_set_refclk;	\
	(__config)->tuner_get_status	= (__ctl)->tuner_get_status;	\
}

static int saa7231_gpio_reset(struct saa7231_dev *saa7231, enum saa7231_gpio gpio, int delay)
{
	if (saa7231_gpio_set(saa7231, gpio, 0) < 0)
		return -EIO;
	msleep(delay);
	if (saa7231_gpio_set(saa7231, gpio, 1) < 0)
		return -EIO;
	msleep(delay);

	return 0;
}

#define SAA7231_IRQNONE(__status)	(!(__status[0]	| \
					   __status[1]	| \
					   __status[2]	| \
					   __status[3]))

#define SAA7231_UNPLUGD(__status)	((__status[0] == 0xffffffff)	&& \
					 (__status[1] == 0xffffffff)	&& \
					 (__status[2] == 0xffffffff)	&& \
					 (__status[3] == 0xffffffff))

static irqreturn_t saa7231_irq_handler(int irq, void *dev_id)
{
	struct saa7231_dev *saa7231 = (struct saa7231_dev *) dev_id;
	struct saa7231_irq_entry *event;

	u32 dcs_stat, dcs_addr, status[4];
	int i, j, vector = 0;

	if (unlikely(!saa7231)) {
		printk("%s: bgt7231=NULL\n", __func__);
		return IRQ_NONE;
	}

	for (i = 0; i < SAA7231_MSI_LOOPS; i++)
		status[i] = SAA7231_RD(SAA7231_BAR0, MSI, MSI_INT_STATUS(i));

	if (SAA7231_IRQNONE(status))
		return IRQ_NONE;

	if (SAA7231_UNPLUGD(status))
		return IRQ_NONE;

	for (i = 0; i < SAA7231_MSI_LOOPS; i++) {

		for (j = 0; j < 32; j++) {
			irq = status[i] >> j;
			if (irq & 0x1) {
				vector = (i * 32) + j;
				event = &saa7231->event_handler[vector];

				SAA7231_WR(status[i], SAA7231_BAR0, MSI, MSI_INT_STATUS_CLR(i));
				if (status[1] & 0x100000) {

					dcs_stat = SAA7231_RD(SAA7231_BAR0, DCSN, DCSN_INT_STATUS);
					dcs_addr = SAA7231_RD(SAA7231_BAR0, DCSN, DCSN_ADDR);
					dprintk(SAA7231_DEBUG, 1, "Clearing access violation (0x%x) @0x%x...", dcs_stat, dcs_addr);

					SAA7231_WR(dcs_stat, SAA7231_BAR0, DCSN, DCSN_INT_CLR_STATUS);
					status[1] &= ~ 0x100000;
				}

				if (event->vector == vector)
					event->handler(saa7231, vector);
			}
		}
	}

	return IRQ_HANDLED;
}


static int saa7231_pci_probe(struct pci_dev *pdev, const struct pci_device_id *pci_id)
{
	struct saa7231_dev *saa7231;
	int err = 0;

	saa7231 = kzalloc(sizeof (struct saa7231_dev), GFP_KERNEL);
	if (!saa7231) {
		printk(KERN_ERR "saa7231_hybrid_pci_probe ERROR: out of memory\n");
		err = -ENOMEM;
		goto fail0;
	}

	saa7231->num		= num;
	saa7231->verbose	= verbose;

	saa7231->int_type	= int_type;
	saa7231->pdev		= pdev;
	saa7231->config		= (struct saa7231_config *) pci_id->driver_data;

	strcpy(saa7231->ver, DRIVER_VER);

	err = saa7231_pci_init(saa7231);
	if (err) {
		dprintk(SAA7231_ERROR, 1, "SAA7231 PCI Initialization failed, err=%d", err);
		goto fail1;
	}

	err = saa7231_cgu_init(saa7231);
	if (err) {
		dprintk(SAA7231_ERROR, 1, "SAA7231 CGU Init failed, err=%d", err);
		goto fail2;
	}

	err = saa7231_msi_init(saa7231);
	if (err) {
		dprintk(SAA7231_ERROR, 1, "SAA7231 MSI Init failed, err=%d", err);
		goto fail3;
	}

	err = saa7231_i2c_init(saa7231);
	if (err) {
		dprintk(SAA7231_ERROR, 1, "SAA7231 I2C Initialization failed, err=%d", err);
		goto fail4;
	}

	err = saa7231_if_init(saa7231);
	if (err) {
		dprintk(SAA7231_ERROR, 1, "SAA7231 IF Initialization failed, err=%d", err);
		goto fail4;
	}
#if 0
	err = saa7231_vfl_init(saa7231);
	if (err) {
		dprintk(SAA7231_ERROR, 1, "SAA7231 VFL initialization failed, err=%d", err);
		goto fail6;
	}
#endif
#if 1
	err = saa7231_dvb_init(saa7231);
	if (err) {
		dprintk(SAA7231_ERROR, 1, "SAA7231 DVB initialization failed, err=%d", err);
		goto fail5;
	}
#endif

#if 0
	err = saa7231_alsa_init(saa7231);
	if (err) {
		dprintk(SAA7231_ERROR, 1, "SAA7231 ALSA initializaton failed, err=%d", err);
		goto fail7;
	}
#endif
	dprintk(SAA7231_DEBUG, 1, "SAA7231 device:%d initialized", num);
	num += 1;
	return 0;

//fail7:
//	saa7231_alsa_exit(saa7231);
//fail6:
//	saa7231_dvb_exit(saa7231);
fail5:
	saa7231_i2c_exit(saa7231);
fail4:
	saa7231_msi_exit(saa7231);
fail3:
	saa7231_cgu_exit(saa7231);
fail2:
	saa7231_pci_exit(saa7231);
fail1:
	kfree(saa7231);

fail0:
	return err;
}

static void saa7231_pci_remove(struct pci_dev *pdev)
{
	struct saa7231_dev *saa7231 = pci_get_drvdata(pdev);
	BUG_ON(!saa7231);

//	saa7231_alsa_exit(saa7231);

//	saa7231_vfl_exit(saa7231);
	saa7231_dvb_exit(saa7231);
	saa7231_i2c_exit(saa7231);
	saa7231_msi_exit(saa7231);
	saa7231_cgu_exit(saa7231);
	saa7231_pci_exit(saa7231);
	kfree(saa7231);
	num -= 1;
}


static struct tda10048_config bgt3575_tda10048_config = {
	.demod_address    = 0x10 >> 1,
	.output_mode      = TDA10048_SERIAL_OUTPUT,
	.fwbulkwritelen   = TDA10048_BULKWRITE_200,
	.inversion        = TDA10048_INVERSION_ON,
	.dtv6_if_freq_khz = TDA10048_IF_3300,
	.dtv7_if_freq_khz = TDA10048_IF_3800,
	.dtv8_if_freq_khz = TDA10048_IF_4300,
	.clk_freq_khz     = TDA10048_CLK_16000,
};

static struct tda18271_std_map bgt3585_tda18271_dvbt = {
	.dvbt_6 = { .if_freq = 3300, .agc_mode = 3, .std = 4, .if_lvl = 1, .rfagc_top = 0x37 },
	.dvbt_7 = { .if_freq = 3800, .agc_mode = 3, .std = 5, .if_lvl = 1, .rfagc_top = 0x37 },
	.dvbt_8 = { .if_freq = 4300, .agc_mode = 3, .std = 6, .if_lvl = 1, .rfagc_top = 0x37 },
};

static struct tda18271_config bgt3575_tda18271_config = {
	.std_map	= &bgt3585_tda18271_dvbt,
	.gate		= TDA18271_GATE_DIGITAL,
};

static struct stv090x_config bgt3575_stv090x_config = {
	.device			= STV0903,
	.demod_mode		= STV090x_SINGLE,
	.clk_mode		= STV090x_CLK_EXT,

	.xtal			= 8000000,
	.address		= 0xd0 >> 1,

	.ts3_clk                = 81000000,
	.ts3_mode		= STV090x_TSMODE_SERIAL_CONTINUOUS,
	.repeater_level		= STV090x_RPTLEVEL_16,
};

static struct stv6110x_config bgt3575_stv6110x_config = {
	.addr			= 0xc6 >> 1,
	.refclk			= 16000000,
	.clk_div		= 2,
};

static u8 bgt3575_lnbh23_config = {
	0x16 >> 1,
};

static struct stv090x_config bgt3576_stv090x_config = {
	.device			= STV0903,
	.demod_mode		= STV090x_SINGLE,
	.clk_mode		= STV090x_CLK_EXT,

	.xtal			= 8000000,
	.address		= 0xd0 >> 1,

	.ts3_clk                = 81000000,
	.ts3_mode		= STV090x_TSMODE_SERIAL_CONTINUOUS,
	.repeater_level		= STV090x_RPTLEVEL_16,
};

static struct stv6110x_config bgt3576_stv6110x_config = {
	.addr			= 0xc6 >> 1,
	.refclk			= 16000000,
	.clk_div		= 2,
};

static u8 bgt3576_lnbh23_config = {
	0x16 >> 1,
};

static struct s5h1411_config bgt3576_s5h1411_config = {
	.output_mode	= S5H1411_SERIAL_OUTPUT,
	.gpio		= S5H1411_GPIO_OFF,
	.vsb_if		= S5H1411_IF_44000,
	.qam_if		= S5H1411_IF_4000,
	.inversion	= S5H1411_INVERSION_ON,
	.status_mode	= S5H1411_DEMODLOCKING,
	.mpeg_timing	= S5H1411_MPEGTIMING_CONTINOUS_NONINVERTING_CLOCK,
};

static struct tda18271_std_map bgt3576_tda18271_atsc = {
	.atsc_6 = { .if_freq = 5380, .agc_mode = 3, .std = 3, .if_lvl = 6, .rfagc_top = 0x37 },
	.qam_6  = { .if_freq = 4000, .agc_mode = 3, .std = 0, .if_lvl = 6, .rfagc_top = 0x37 },
};

static struct tda18271_config bgt3576_tda18271_config = {
	.std_map	= &bgt3576_tda18271_atsc,
	.gate		= TDA18271_GATE_DIGITAL,
};

static struct tda10048_config bgt3585_tda10048_config[] = {
	{
		.demod_address    = 0x10 >> 1,
		.output_mode      = TDA10048_SERIAL_OUTPUT,
		.fwbulkwritelen   = TDA10048_BULKWRITE_200,
		.inversion        = TDA10048_INVERSION_ON,
		.dtv6_if_freq_khz = TDA10048_IF_3300,
		.dtv7_if_freq_khz = TDA10048_IF_3800,
		.dtv8_if_freq_khz = TDA10048_IF_4300,
		.clk_freq_khz     = TDA10048_CLK_16000,
	}, {
		.demod_address    = 0x12 >> 1,
		.output_mode      = TDA10048_SERIAL_OUTPUT,
		.fwbulkwritelen   = TDA10048_BULKWRITE_200,
		.inversion        = TDA10048_INVERSION_ON,
		.dtv6_if_freq_khz = TDA10048_IF_3300,
		.dtv7_if_freq_khz = TDA10048_IF_3800,
		.dtv8_if_freq_khz = TDA10048_IF_4300,
		.clk_freq_khz     = TDA10048_CLK_16000,
	}
};

static struct tda18271_config bgt3585_tda18271_config = {
	.std_map	= &bgt3585_tda18271_dvbt,
	.gate		= TDA18271_GATE_DIGITAL,
};

static struct stv090x_config bgt3595_stv090x_config = {
	.device			= STV0900,
	.demod_mode		= STV090x_DUAL,
	.clk_mode		= STV090x_CLK_EXT,

	.xtal			= 8000000,
	.address		= 0xd0 >> 1,

	.repeater_level		= STV090x_RPTLEVEL_16,

	.adc1_range		= STV090x_ADC_1Vpp,
	.adc2_range		= STV090x_ADC_1Vpp,

	.ts1_mode		= STV090x_TSMODE_SERIAL_PUNCTURED,
	.ts2_mode		= STV090x_TSMODE_SERIAL_PUNCTURED,
};

static struct stv6110x_config bgt3595_stv6110x_config = {
	.addr			= 0xC6 >> 1,
	.refclk			= 16000000,
	.clk_div		= 2,
};

static u8 bgt3595_lnbh24_config[] = {
	0x12 >> 1,
	0x14 >> 1,
};

static struct tda18271_config purus_mpcie_tda18271_config = {
	.gate		= TDA18271_GATE_DIGITAL,
};

static struct s5h1411_config purus_mpcie_s5h1411_config = {
	.output_mode	= S5H1411_SERIAL_OUTPUT,
	.gpio		= S5H1411_GPIO_OFF,
	.vsb_if		= S5H1411_IF_5380,
	.qam_if		= S5H1411_IF_4000,
	.inversion	= S5H1411_INVERSION_ON,
	.status_mode	= S5H1411_DEMODLOCKING,
	.mpeg_timing	= S5H1411_MPEGTIMING_CONTINOUS_NONINVERTING_CLOCK,
};

static struct tda18272_config bgt3620_tda18272_config[] = {
	{
		.addr		= (0xc0 >> 1),
		.mode		= TDA18272_MASTER,
	}, {
		.addr		= (0xc0 >> 1),
		.mode		= TDA18272_SLAVE,
	}
};


static struct cxd2820r_config bgt3620_cxd2820r_config = {
	.i2c_address	= (0xd8 >> 1),
	.ts_mode	= CXD2820R_TS_SERIAL,
};

static struct cxd2850_config bgt3636_cxd2850_config = {
	.xtal		= 27000000,
	.dmd_addr	= (0xd0 >> 1),
	.tnr_addr	= (0xc0 >> 1),
	.shared_xtal	= 1,
	.serial_ts	= 1,
	.serial_d0	= 0,
	.tsclk_pol	= 1,
	.tone_out	= 1,
};

static struct a8290_config bgt3636_a8290_config = {
	.address	= (0x10 >> 1),
};

static struct cxd2817_config bgt3636_cxd2817_config = {
	.xtal		= 41000000,
	.addr		= (0xd8 >> 1),
	.serial_ts	= 1,
};

static struct cxd2861_cfg bgt3636_cxd2861_config = {
	.address	= (0xc0 >> 1),

	.ext_osc	= 1,
	.f_xtal		= 41,
};

#define NXP				"NXP Semiconductor"
#define PURUS_PCIe_REF			0x0001
#define PURUS_PCI_REF			0x0002
#define PURUS_mPCIe_REF			0x0003


#define BLACKGOLD			"Blackgold Technology"
#define BLACKGOLD_TECHNOLOGY		0x14c7
#define BLACKGOLD_BGT3575		0x3575
#define BLACKGOLD_BGT3576		0x3576
#define BLACKGOLD_BGT3585		0x3585
#define BLACKGOLD_BGT3596		0x3596
#define BLACKGOLD_BGT3595		0x3595
#define BLACKGOLD_BGT3600		0x3600
#define BLACKGOLD_BGT3620		0x3620
#define BLACKGOLD_BGT3630		0x3630
#define BLACKGOLD_BGT3650		0x3650
#define BLACKGOLD_BGT3651		0x3651
#define BLACKGOLD_BGT3660		0x3660
#define BLACKGOLD_BGT3685		0x3685
#define BLACKGOLD_BGT3695		0x3695
#define BLACKGOLD_BGT3696		0x3696
#define BLACKGOLD_BGT3636		0x3636

#define SUBVENDOR_ALL			0x0000
#define SUBDEVICE_ALL			0x0000

#define MAKE_DESC(__vendor, __product, __type) {	\
	.vendor		= (__vendor),			\
	.product	= (__product),			\
	.type		= (__type),			\
}

#define PURUS_PCIE			0
#define PURUS_MPCIE			1
#define PURUS_PCI			2

#define BGT3595				3
#define BGT3596				4
#define BGT3585				5
#define BGT3576				6
#define BGT3575				7
#define BGT3600				8
#define BGT3620				9
#define BGT3630			       10
#define BGT3650			       11
#define BGT3651			       12
#define BGT3660			       13
#define BGT3685			       14
#define BGT3695			       15
#define BGT3696			       16
#define BGT3636			       17

static struct card_desc saa7231_desc[] = {
	MAKE_DESC(NXP,		"Purus PCIe",	"DVB-S + DVB-T + Analog Ref. design"),
	MAKE_DESC(NXP,		"Purus mPCIe",	"DVB-T + ATSC + Analog Ref. design"),
	MAKE_DESC(NXP,		"Purus PCI",	"DVB-S + DVB-T + Analog Ref. design"),

	MAKE_DESC(BLACKGOLD,	"BGT3595", 	"Dual DVB-S/S2 + Analog"),
	MAKE_DESC(BLACKGOLD,	"BGT3596", 	"Dual ATSC"),
	MAKE_DESC(BLACKGOLD,	"BGT3585", 	"Dual DVB-T/H + Analog"),
	MAKE_DESC(BLACKGOLD,	"BGT3576", 	"DVB-S/S2 + ATSC"),
	MAKE_DESC(BLACKGOLD,	"BGT3575", 	"DVB-S/S2 + DVB-T/H"),
	MAKE_DESC(BLACKGOLD,	"BGT3600",	"DVB-T/T2 + Analog"),
	MAKE_DESC(BLACKGOLD,	"BGT3620",	"Dual DVB-T/T2/C + Analog"),
	MAKE_DESC(BLACKGOLD,	"BGT3630",	"DVB-T/T2 + DVB-S/S2 + Analog"),
	MAKE_DESC(BLACKGOLD,	"BGT3650",	"Dual DVB-T/T2 + Analog"),
	MAKE_DESC(BLACKGOLD,	"BGT3651",	"Dual DVB-T/T2 + Analog"),
	MAKE_DESC(BLACKGOLD,	"BGT3660",	"Dual DVB-T/T2 + Analog"),
	MAKE_DESC(BLACKGOLD,	"BGT3685",	"Dual DVB-T + Analog"),
	MAKE_DESC(BLACKGOLD,	"BGT3695",	"Dual DVB-T + Analog"),
	MAKE_DESC(BLACKGOLD,	"BGT3696",	"Dual ATSC + Analog"),
	MAKE_DESC(BLACKGOLD,	"BGT3636",	"DVB-S/S2 + DVB-T/T2/C + Analog"),
	{ }
};

#define SUBSYS_INFO(__vendor, __device) ((__vendor << 16) | __device)
#define DEVICE_DESC(__devid)	(&saa7231_desc[(__devid)])

static int saa7231_frontend_enable(struct saa7231_dev *saa7231)
{
	struct pci_dev *pdev		= saa7231->pdev;
	u32 subsystem_info		= SUBSYS_INFO(pdev->subsystem_vendor, pdev->subsystem_device);
	int ret = 0;

	switch (subsystem_info) {
	case SUBSYS_INFO(NXP_REFERENCE_BOARD, PURUS_PCIe_REF):
		break;
	case SUBSYS_INFO(NXP_REFERENCE_BOARD, PURUS_mPCIe_REF):
		GPIO_SET_OUT(GPIO_3 | GPIO_4 | GPIO_5);
		if (saa7231_gpio_set(saa7231, GPIO_3, 0) < 0)
			ret = -EIO;
		msleep(100);
		if (saa7231_gpio_reset(saa7231, GPIO_5, 10) < 0)
			ret = -EIO;
		if (saa7231_gpio_reset(saa7231, GPIO_4, 10) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3595):
		GPIO_SET_OUT(GPIO_1);
		if (saa7231_gpio_reset(saa7231, GPIO_0, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3585):
		GPIO_SET_OUT(GPIO_1);
		if (saa7231_gpio_reset(saa7231, GPIO_0, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3576):
		GPIO_SET_OUT(GPIO_1 | GPIO_2);
		if (saa7231_gpio_reset(saa7231, GPIO_2, 50) < 0)
			ret = -EIO;
		if (saa7231_gpio_reset(saa7231, GPIO_1, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3575):
		GPIO_SET_OUT(GPIO_1 | GPIO_2);
		if (saa7231_gpio_reset(saa7231, GPIO_1, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3600):
		GPIO_SET_OUT(GPIO_1);
		if (saa7231_gpio_reset(saa7231, GPIO_1, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3620):
		GPIO_SET_OUT(GPIO_2);
		if (saa7231_gpio_reset(saa7231, GPIO_2, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3630):
		GPIO_SET_OUT(GPIO_1 | GPIO_2);
		if (saa7231_gpio_reset(saa7231, GPIO_1, 50) < 0)
			ret = -EIO;
		if (saa7231_gpio_reset(saa7231, GPIO_2, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3650):
		GPIO_SET_OUT(GPIO_1);
		if (saa7231_gpio_reset(saa7231, GPIO_1, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3651):
		GPIO_SET_OUT(GPIO_1);
		if (saa7231_gpio_reset(saa7231, GPIO_1, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3660):
		GPIO_SET_OUT(GPIO_1);
		if (saa7231_gpio_reset(saa7231, GPIO_1, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3685):
		GPIO_SET_OUT(GPIO_1);
		if (saa7231_gpio_reset(saa7231, GPIO_1, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3695):
		GPIO_SET_OUT(GPIO_1);
		if (saa7231_gpio_reset(saa7231, GPIO_1, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3696):
		GPIO_SET_OUT(GPIO_1);
		if (saa7231_gpio_reset(saa7231, GPIO_1, 50) < 0)
			ret = -EIO;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3636):
		GPIO_SET_OUT(GPIO_1 | GPIO_2);
		if (saa7231_gpio_reset(saa7231, GPIO_1, 50) < 0)
			ret = -EIO;
		if (saa7231_gpio_reset(saa7231, GPIO_2, 50) < 0)
			ret = -EIO;
		break;
	}
	return ret;
}


static int saa7231_frontend_attach(struct saa7231_dvb *dvb, int frontend)
{
	struct saa7231_dev *saa7231	= dvb->saa7231;
	struct pci_dev *pdev		= saa7231->pdev;
	u32 subsystem_info		= SUBSYS_INFO(pdev->subsystem_vendor, pdev->subsystem_device);

	struct saa7231_i2c *i2c_0	= &saa7231->i2c[0];
	struct saa7231_i2c *i2c_1	= &saa7231->i2c[1];
	struct saa7231_i2c *i2c_2	= &saa7231->i2c[2];

	struct saa7231_i2c *i2c;
	struct stv6110x_devctl *ctl;
	int ret = 0;

	dprintk(SAA7231_DEBUG, 1, "Frontend Init Adapter (%d) Device ID=%02x",
		frontend,
		saa7231->pdev->subsystem_device);

	switch (subsystem_info) {
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3636):
		dprintk(SAA7231_ERROR, 1, "BGT3636 Found .. !");
		if (frontend == 1) {
			dvb->fe = dvb_attach(cxd2850_attach,
					     &bgt3636_cxd2850_config,
					     &saa7231->i2c[frontend].i2c_adapter);

			if (!dvb->fe) {
				dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
				ret = -ENODEV;
				goto exit;
			}
			dvb_attach(a8290_attach,
				   dvb->fe,
				   &bgt3636_a8290_config,
				   &saa7231->i2c[frontend - 1].i2c_adapter);
		}
		if (frontend == 0) {
			dvb->fe = dvb_attach(cxd2817_attach,
					     &bgt3636_cxd2817_config,
					     &saa7231->i2c[2 + frontend].i2c_adapter);

			if (!dvb->fe) {
				dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
				ret = -ENODEV;
				goto exit;
			}
			dvb_attach(cxd2861_attach,
				   dvb->fe,
				   &bgt3636_cxd2861_config,
				   &saa7231->i2c[2 + frontend].i2c_adapter);
		}
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3651):
		dvb->fe = dvb_attach(cxd2820r_attach,
				     &bgt3620_cxd2820r_config,
				     &saa7231->i2c[1 + frontend].i2c_adapter,
				     NULL);

		if (!dvb->fe) {
			dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
			ret = -ENODEV;
			goto exit;
		} else {
			dvb_attach(tda18272_attach,
				   dvb->fe,
				   &saa7231->i2c[1 + frontend].i2c_adapter,
				   &bgt3620_tda18272_config[frontend]);
		}
		ret = 0;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3650):
		dvb->fe = dvb_attach(cxd2820r_attach,
				     &bgt3620_cxd2820r_config,
				     &saa7231->i2c[1 + frontend].i2c_adapter,
				     NULL);

		if (!dvb->fe) {
			dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
			ret = -ENODEV;
			goto exit;
		} else {
			dvb_attach(tda18272_attach,
				   dvb->fe,
				   &saa7231->i2c[1 + frontend].i2c_adapter,
				   &bgt3620_tda18272_config[frontend]);
		}
		ret = 0;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3630):
		if (frontend == 1) {
			dvb->fe = dvb_attach(stv090x_attach,
					     &bgt3575_stv090x_config,
					     &i2c_1->i2c_adapter,
					     STV090x_DEMODULATOR_0);
			if (!dvb->fe) {
				dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
				ret = -ENODEV;
				goto exit;

			}
			ctl = dvb_attach(stv6110x_attach,
					 dvb->fe,
					 &bgt3575_stv6110x_config,
					 &i2c_1->i2c_adapter);

			if (!ctl) {
				dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
				ret = -ENODEV;
				goto exit;
			}
			STV090x_CFGUPDATE(&bgt3575_stv090x_config, ctl);
			if (dvb->fe->ops.init)
				dvb->fe->ops.init(dvb->fe);

			if (dvb_attach(lnbh24_attach,
				       dvb->fe,
				       &i2c_0->i2c_adapter,
				       0,
				       0,
				       bgt3575_lnbh23_config) == NULL) {

				dprintk(SAA7231_ERROR, 1, "LNBH23 not found");
				goto exit;
			}
		}
		if (frontend == 0) {
			dvb->fe = dvb_attach(cxd2820r_attach,
					     &bgt3620_cxd2820r_config,
					     &saa7231->i2c[2 + frontend].i2c_adapter,
					     NULL);
			if (!dvb->fe) {
				dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
				ret = -ENODEV;
				goto exit;
			} else {
				dvb_attach(tda18272_attach,
					   dvb->fe,
					   &saa7231->i2c[2 + frontend].i2c_adapter,
					   bgt3620_tda18272_config);
			}
		}
		ret = 0;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3620):
#if 1
		dvb->fe = dvb_attach(cxd2820r_attach,
				     &bgt3620_cxd2820r_config,
				     &saa7231->i2c[1 + frontend].i2c_adapter,
				     NULL);

		if (!dvb->fe) {
			dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
			ret = -ENODEV;
			goto exit;
		} else {
			dvb_attach(tda18272_attach,
				   dvb->fe,
				   &saa7231->i2c[1 + frontend].i2c_adapter,
				   &bgt3620_tda18272_config[frontend]);
		}
#endif
#if 0
		dvb->fe = dvb_attach(cxd2834_attach,
				     NULL,
				     &saa7231->i2c[1 + frontend].i2c_adapter,
				     &bgt3620_cxd2834_config);

		if (!dvb->fe) {
			dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
			ret = -ENODEV;
			goto exit;
		} else {
			dvb_attach(tda18272_attach,
				   dvb->fe,
				   &saa7231->i2c[1 + frontend].i2c_adapter,
				   &bgt3620_tda18272_config[frontend]);
		}
#endif
		ret = 0;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3600):
		dvb->fe = dvb_attach(cxd2820r_attach,
				     &bgt3620_cxd2820r_config,
				     &saa7231->i2c[1 + frontend].i2c_adapter,
				     NULL);

		if (!dvb->fe) {
			dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
			ret = -ENODEV;
			goto exit;
		} else {
			dvb_attach(tda18272_attach,
				   dvb->fe,
				   &saa7231->i2c[1 + frontend].i2c_adapter,
				   &bgt3620_tda18272_config[frontend]);
		}
		ret = 0;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3595):
		dvb->fe = dvb_attach(stv090x_attach,
				     &bgt3595_stv090x_config,
				     &i2c_1->i2c_adapter,
				     STV090x_DEMODULATOR_0 + frontend);

		if (!dvb->fe) {
			dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
			ret = -ENODEV;
			goto exit;
		}
		ctl = dvb_attach(stv6110x_attach,
				 dvb->fe,
				 &bgt3595_stv6110x_config,
				 &i2c_1->i2c_adapter);

		STV090x_CFGUPDATE(&bgt3595_stv090x_config, ctl);
		if (dvb->fe->ops.init)
			dvb->fe->ops.init(dvb->fe);

		dvb_attach(lnbh24_attach,
			   dvb->fe,
			   &i2c_0->i2c_adapter,
			   0,
			   0,
			   bgt3595_lnbh24_config[frontend]);
		ret = 0;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3585):
		i2c = &saa7231->i2c[frontend + 1];
		dvb->fe = dvb_attach(tda10048_attach,
				     &bgt3585_tda10048_config[frontend],
				     &i2c->i2c_adapter);

		if (!dvb->fe) {
			dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
			ret = -ENODEV;
			goto exit;
		}
		dvb_attach(tda18271_attach,
			   dvb->fe,
			   0x60,
			   &i2c->i2c_adapter,
			   &bgt3585_tda18271_config);
		ret = 0;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3576):
		if (frontend == 0) {
			dvb->fe = dvb_attach(s5h1411_attach,
					     &bgt3576_s5h1411_config,
					     &i2c_2->i2c_adapter);

			if (!dvb->fe) {
				dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
				ret = -ENODEV;
				goto exit;
			}
			dvb_attach(tda18271_attach,
				   dvb->fe,
				   (0xc0 >> 1),
				   &i2c_2->i2c_adapter,
				   &bgt3576_tda18271_config);
		}
		if (frontend == 1) {
			dvb->fe = dvb_attach(stv090x_attach,
					     &bgt3576_stv090x_config,
					     &i2c_1->i2c_adapter,
					     STV090x_DEMODULATOR_0);

			if (!dvb->fe) {
				dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
				ret = -ENODEV;
				goto exit;
			}
			ctl = dvb_attach(stv6110x_attach,
					 dvb->fe,
					 &bgt3576_stv6110x_config,
					 &i2c_1->i2c_adapter);

			if (!ctl) {
				dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
				ret = -ENODEV;
				goto exit;
			}
			STV090x_CFGUPDATE(&bgt3576_stv090x_config, ctl);
			if (dvb->fe->ops.init)
				dvb->fe->ops.init(dvb->fe);

			if (dvb_attach(lnbh24_attach,
				dvb->fe,
				&i2c_0->i2c_adapter,
				0,
				0,
				bgt3576_lnbh23_config) == NULL) {

				dprintk(SAA7231_ERROR, 1, "LNBH23 not found");
				goto exit;
			}
		}
		ret = 0;
		break;
	case SUBSYS_INFO(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3575):
		if (frontend == 0) {
			dvb->fe = dvb_attach(tda10048_attach,
					     &bgt3575_tda10048_config,
					     &i2c_2->i2c_adapter);

			if (!dvb->fe) {
				dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
				ret = -ENODEV;
				goto exit;
			}
			dvb_attach(tda18271_attach,
				   dvb->fe,
				   0x60,
				   &i2c_2->i2c_adapter,
				   &bgt3575_tda18271_config);
		}
		if (frontend == 1) {
			dvb->fe = dvb_attach(stv090x_attach,
					     &bgt3575_stv090x_config,
					     &i2c_1->i2c_adapter,
					     STV090x_DEMODULATOR_0);

			if (!dvb->fe) {
				dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
				ret = -ENODEV;
				goto exit;

			}

			ctl = dvb_attach(stv6110x_attach,
					 dvb->fe,
					 &bgt3575_stv6110x_config,
					 &i2c_1->i2c_adapter);

			if (!ctl) {
				dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
				ret = -ENODEV;
				goto exit;
			}
			STV090x_CFGUPDATE(&bgt3575_stv090x_config, ctl);
			if (dvb->fe->ops.init)
				dvb->fe->ops.init(dvb->fe);

			if (dvb_attach(lnbh24_attach,
				       dvb->fe,
				       &i2c_0->i2c_adapter,
				       0,
				       0,
				       bgt3575_lnbh23_config) == NULL) {

				dprintk(SAA7231_ERROR, 1, "LNBH23 not found");
				goto exit;
			}

		}
		ret = 0;
		break;
	case SUBSYS_INFO(NXP_REFERENCE_BOARD, PURUS_PCIe_REF):
		break;
	case SUBSYS_INFO(NXP_REFERENCE_BOARD, PURUS_mPCIe_REF):
		dvb->fe = dvb_attach(s5h1411_attach,
				     &purus_mpcie_s5h1411_config,
				     &i2c_0->i2c_adapter);

		if (!dvb->fe) {
			dprintk(SAA7231_ERROR, 1, "Frontend:%d attach failed", frontend);
			ret = -ENODEV;
			goto exit;
		}

		dvb_attach(tda18271_attach,
			   dvb->fe,
			   0x60,
			   &i2c_1->i2c_adapter,
			   &purus_mpcie_tda18271_config);

		ret = 0;
		break;
	}
exit:
	return ret;
}

static struct saa7231_config purus_blackgold_bgt3636 = {
	.desc			= DEVICE_DESC(BGT3636),
	.xtal			= 54,
	.i2c_rate		= SAA7231_I2C_RATE_400,
	.root_clk		= CLK_ROOT_54MHz,
	.irq_handler		= saa7231_irq_handler,

	.ext_dvb_adapters	= 2,
	.ts0_cfg		= 0x41,
	.ts0_clk		= 0x05,
	.ts1_cfg		= 0x41,
	.ts1_clk		= 0x05,

	.frontend_enable	= saa7231_frontend_enable,
	.frontend_attach	= saa7231_frontend_attach,

	.stream_ports		= 2,
};

static struct saa7231_config purus_blackgold_bgt3630 = {
	.desc			= DEVICE_DESC(BGT3630),
	.xtal			= 54,
	.i2c_rate		= SAA7231_I2C_RATE_100,
	.root_clk		= CLK_ROOT_54MHz,
	.irq_handler		= saa7231_irq_handler,

	.ext_dvb_adapters	= 2,
	.ts0_cfg		= 0x41,
	.ts0_clk		= 0x01,
	.ts1_cfg		= 0x41,
	.ts1_clk		= 0x05,

	.frontend_enable	= saa7231_frontend_enable,
	.frontend_attach	= saa7231_frontend_attach,

	.stream_ports		= 2,
};

static struct saa7231_config purus_blackgold_bgt3600 = {
	.desc			= DEVICE_DESC(BGT3600),

	.xtal			= 54,
	.i2c_rate		= SAA7231_I2C_RATE_100,
	.root_clk		= CLK_ROOT_54MHz,
	.irq_handler		= saa7231_irq_handler,

	.ext_dvb_adapters	= 2,
	.ts0_cfg		= 0x41,
	.ts0_clk		= 0x05,
	.ts1_cfg		= 0x41,
	.ts1_clk		= 0x05,
	.frontend_enable	= saa7231_frontend_enable,
	.frontend_attach	= saa7231_frontend_attach,

	.stream_ports		= 2,
};

static struct saa7231_config purus_blackgold_bgt3651 = {
	.desc			= DEVICE_DESC(BGT3651),

	.xtal			= 54,
	.i2c_rate		= SAA7231_I2C_RATE_100,
	.root_clk		= CLK_ROOT_54MHz,
	.irq_handler		= saa7231_irq_handler,

	.ext_dvb_adapters	= 2,
	.ts0_cfg		= 0x41,
	.ts0_clk		= 0x01,
	.ts1_cfg		= 0x41,
	.ts1_clk		= 0x01,
	.frontend_enable	= saa7231_frontend_enable,
	.frontend_attach	= saa7231_frontend_attach,

	.stream_ports		= 2,
};

static struct saa7231_config purus_blackgold_bgt3650 = {
	.desc			= DEVICE_DESC(BGT3650),

	.xtal			= 54,
	.i2c_rate		= SAA7231_I2C_RATE_100,
	.root_clk		= CLK_ROOT_54MHz,
	.irq_handler		= saa7231_irq_handler,

	.ext_dvb_adapters	= 2,
	.ts0_cfg		= 0x41,
	.ts0_clk		= 0x01,
	.ts1_cfg		= 0x41,
	.ts1_clk		= 0x01,
	.frontend_enable	= saa7231_frontend_enable,
	.frontend_attach	= saa7231_frontend_attach,

	.stream_ports		= 2,
};

static struct saa7231_config purus_blackgold_bgt3620 = {
	.desc			= DEVICE_DESC(BGT3620),

	.xtal			= 54,
	.i2c_rate		= SAA7231_I2C_RATE_100,
	.root_clk		= CLK_ROOT_54MHz,
	.irq_handler		= saa7231_irq_handler,

	.ext_dvb_adapters	= 2,
	.ts0_cfg		= 0x41,
	.ts0_clk		= 0x01,
	.ts1_cfg		= 0x41,
	.ts1_clk		= 0x01,
	.frontend_enable	= saa7231_frontend_enable,
	.frontend_attach	= saa7231_frontend_attach,

	.stream_ports		= 2,
};

static struct saa7231_config purus_blackgold_bgt3595 = {

	.desc			= DEVICE_DESC(BGT3595),

	.xtal			= 54,
	.i2c_rate		= SAA7231_I2C_RATE_100,
	.root_clk		= CLK_ROOT_54MHz,
	.irq_handler		= saa7231_irq_handler,

	.ext_dvb_adapters	= 2,
	.ts0_cfg		= 0x41,
	.ts0_clk		= 0x07,
	.ts1_cfg		= 0x41,
	.ts1_clk		= 0x07,
	.frontend_enable	= saa7231_frontend_enable,
	.frontend_attach	= saa7231_frontend_attach,

	.stream_ports		= 2,
};

static struct saa7231_config purus_blackgold_bgt3585 = {

	.desc			= DEVICE_DESC(BGT3585),

	.xtal			= 54,
	.i2c_rate		= SAA7231_I2C_RATE_400,
	.root_clk		= CLK_ROOT_54MHz,
	.irq_handler		= saa7231_irq_handler,

	.ext_dvb_adapters	= 2,
	.ts0_cfg		= 0x41,
	.ts0_clk		= 0x01,
	.ts1_cfg		= 0x41,
	.ts1_clk		= 0x01,
	.frontend_enable	= saa7231_frontend_enable,
	.frontend_attach	= saa7231_frontend_attach,

	.stream_ports		= 2,
};

static struct saa7231_config purus_blackgold_bgt3576 = {

	.desc			= DEVICE_DESC(BGT3576),

	.xtal			= 54,
	.i2c_rate		= SAA7231_I2C_RATE_100,
	.root_clk		= CLK_ROOT_54MHz,
	.irq_handler		= saa7231_irq_handler,

	.ext_dvb_adapters	= 2,
	.ts0_cfg		= 0x41,
	.ts0_clk		= 0x01,
	.ts1_cfg		= 0x41,
	.ts1_clk		= 0x07,
	.frontend_enable	= saa7231_frontend_enable,
	.frontend_attach	= saa7231_frontend_attach,
	.stream_ports		= 2,
};

static struct saa7231_config purus_blackgold_bgt3575 = {

	.desc			= DEVICE_DESC(BGT3575),

	.xtal			= 54,
	.i2c_rate		= SAA7231_I2C_RATE_100,
	.root_clk		= CLK_ROOT_54MHz,
	.irq_handler		= saa7231_irq_handler,

	.ext_dvb_adapters	= 2,
	.ts0_cfg		= 0x41,
	.ts0_clk		= 0x01,
	.ts1_cfg		= 0x41,
	.ts1_clk		= 0x07,
	.frontend_enable	= saa7231_frontend_enable,
	.frontend_attach	= saa7231_frontend_attach,
	.stream_ports		= 2,
};
static struct saa7231_config purus_pcie_ref_config = {

	.desc			= DEVICE_DESC(PURUS_PCIE),

	.a_tvc			= 1,
	.v_cap			= 1,
	.a_cap			= 1,

	.xtal			= 54,
	.i2c_rate		= SAA7231_I2C_RATE_100,
	.root_clk		= CLK_ROOT_54MHz,
	.irq_handler		= saa7231_irq_handler,

	.frontend_enable	= saa7231_frontend_enable,
	.frontend_attach	= saa7231_frontend_attach,
	.stream_ports		= 2,
};

static struct saa7231_config purus_mpcie_ref_config = {

	.desc			= DEVICE_DESC(PURUS_MPCIE),

	.a_tvc			= 1,
	.v_cap			= 1,
	.a_cap			= 1,

	.xtal			= 54,
	.i2c_rate		= SAA7231_I2C_RATE_100,
	.root_clk		= CLK_ROOT_54MHz,
	.irq_handler		= saa7231_irq_handler,

	.ext_dvb_adapters	= 1,
	.ts0_cfg		= 0x11,
	.frontend_enable	= saa7231_frontend_enable,
	.frontend_attach	= saa7231_frontend_attach,
	.stream_ports		= 1,
};

static struct pci_device_id saa7231_pci_table[] = {

	MAKE_ENTRY(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3575, SAA7231, &purus_blackgold_bgt3575),
	MAKE_ENTRY(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3576, SAA7231, &purus_blackgold_bgt3576),
	MAKE_ENTRY(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3585, SAA7231, &purus_blackgold_bgt3585),
	MAKE_ENTRY(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3595, SAA7231, &purus_blackgold_bgt3595),

	MAKE_ENTRY(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3600, SAA7231, &purus_blackgold_bgt3600),
	MAKE_ENTRY(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3620, SAA7231, &purus_blackgold_bgt3620),
	MAKE_ENTRY(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3630, SAA7231, &purus_blackgold_bgt3630),
	MAKE_ENTRY(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3650, SAA7231, &purus_blackgold_bgt3650),
	MAKE_ENTRY(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3651, SAA7231, &purus_blackgold_bgt3651),
	MAKE_ENTRY(BLACKGOLD_TECHNOLOGY, BLACKGOLD_BGT3636, SAA7231, &purus_blackgold_bgt3636),

	MAKE_ENTRY(SUBVENDOR_ALL, SUBDEVICE_ALL, SAA7231, &purus_blackgold_bgt3576),
	MAKE_ENTRY(SUBVENDOR_ALL, SUBDEVICE_ALL, SAA7231, &purus_blackgold_bgt3575),

	MAKE_ENTRY(NXP_REFERENCE_BOARD, PURUS_mPCIe_REF, SAA7231, &purus_mpcie_ref_config),
	MAKE_ENTRY(NXP_REFERENCE_BOARD, PURUS_PCIe_REF, SAA7231, &purus_pcie_ref_config),
	{ }
};
MODULE_DEVICE_TABLE(pci, saa7231_pci_table);

static struct pci_driver saa7231_pci_driver = {
	.name			= DRIVER_NAME,
	.id_table		= saa7231_pci_table,
	.probe			= saa7231_pci_probe,
	.remove			= saa7231_pci_remove,
};

static int saa7231_init(void)
{
	return pci_register_driver(&saa7231_pci_driver);
}

static void saa7231_exit(void)
{
	return pci_unregister_driver(&saa7231_pci_driver);
}

module_init(saa7231_init);
module_exit(saa7231_exit);

MODULE_DESCRIPTION("SAA7231 driver");
MODULE_AUTHOR("Manu Abraham");
MODULE_LICENSE("GPL");
