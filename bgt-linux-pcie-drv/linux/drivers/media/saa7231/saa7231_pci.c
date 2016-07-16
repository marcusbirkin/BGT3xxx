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
#include <asm/io.h>
#include <asm/atomic.h>
#include <linux/spinlock_types.h>
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4, 4, 0)
#include <asm/pgtable.h>
#endif
#include <asm/page.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/delay.h>

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/ioport.h>

#include "saa7231_mod.h"
#include "saa7231_priv.h"
#include "saa7231_msi.h"
#include "saa7231_if_reg.h"


#define DRIVER_NAME				"SAA7231"

static const char *chip_name[] = {
	/* LBGA 256 */
	"SAA7231E",
	"SAA7231AE",
	"SAA7231BE",
	"SAA7231CE",
	"SAA7231HE",
	"SAA7231JE",
	"SAA7231KE",

	/* LFBGA 136 */
	"SAA7231DE",
	"SAA7231FE",
	"SAA7231GE",
	"SAA7231ME",
	"SAA7231NE"
};

static const struct saa7231_caps chip_caps[] = {
	/* LBGA 256 */
	{ .id = SAA7231E,  .pci = 1, .pcie = 1, .ca = 1, .btsc = 1, .nicam = 1, .dvbs = 1, .dvbt = 1 },
	{ .id = SAA7231AE, .pci = 1, .pcie = 0, .ca = 1, .btsc = 0, .nicam = 1, .dvbs = 1, .dvbt = 1 },
	{ .id = SAA7231BE, .pci = 0, .pcie = 1, .ca = 1, .btsc = 0, .nicam = 1, .dvbs = 0, .dvbt = 1 },
	{ .id = SAA7231CE, .pci = 0, .pcie = 1, .ca = 1, .btsc = 0, .nicam = 1, .dvbs = 1, .dvbt = 0 },
	{ .id = SAA7231HE, .pci = 1, .pcie = 0, .ca = 1, .btsc = 0, .nicam = 1, .dvbs = 0, .dvbt = 1 },
	{ .id = SAA7231JE, .pci = 1, .pcie = 0, .ca = 1, .btsc = 0, .nicam = 1, .dvbs = 1, .dvbt = 0 },
	{ .id = SAA7231KE, .pci = 1, .pcie = 0, .ca = 1, .btsc = 0, .nicam = 1, .dvbs = 0, .dvbt = 0 },
	/* LFBGA 136 */
	{ .id = SAA7231DE, .pci = 0, .pcie = 1, .ca = 0, .btsc = 0, .nicam = 1, .dvbs = 0, .dvbt = 1 },
	{ .id = SAA7231FE, .pci = 0, .pcie = 1, .ca = 0, .btsc = 1, .nicam = 1, .dvbs = 0, .dvbt = 0 },
	{ .id = SAA7231GE, .pci = 0, .pcie = 1, .ca = 0, .btsc = 0, .nicam = 1, .dvbs = 0, .dvbt = 0 },
	{ .id = SAA7231ME, .pci = 0, .pcie = 1, .ca = 0, .btsc = 0, .nicam = 1, .dvbs = 1, .dvbt = 0 },
	{ .id = SAA7231NE, .pci = 0, .pcie = 1, .ca = 0, .btsc = 0, .nicam = 1, .dvbs = 0, .dvbt = 1 },
};

static irqreturn_t saa7231_irq_handler(int irq, void *dev_id)
{
	struct saa7231_dev *saa7231	= (struct saa7231_dev *) dev_id;

	if (unlikely(!saa7231)) {
		printk("%s: saa7231=NULL\n", __func__);
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static irqreturn_t saa7231_msi_handler(int irq, void *dev_id)
{
	struct saa7231_dev *saa7231	= (struct saa7231_dev *) dev_id;

	if (unlikely(!saa7231)) {
		printk("%s: saa7231=NULL\n", __func__);
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static struct saa7231_msix_entry saa7231_msix_handler[] = {
	{ .desc = "SAA7231_HANDLER", .handler = saa7231_irq_handler }
};

static int saa7231_enable_msi(struct saa7231_dev *saa7231)
{
	struct pci_dev *pdev = saa7231->pdev;
	int err;

	err = pci_enable_msi(pdev);
	if (err) {
		dprintk(SAA7231_ERROR, 1, "MSI enable failed <%d>", err);
		return err;
	}

	return err;
}

static int saa7231_enable_msix(struct saa7231_dev *saa7231)
{
	struct pci_dev *pdev = saa7231->pdev;
	int i, ret = 0;

	for (i = 0; i < saa7231->msi_vectors_max; i++)
		saa7231->msix_entries[i].entry = i;

	ret = pci_enable_msix(pdev, saa7231->msix_entries, saa7231->msi_vectors_max);
	if (ret < 0)
		dprintk(SAA7231_ERROR, 1, "MSI-X request failed");
	if (ret > 0)
		dprintk(SAA7231_ERROR, 1, "Request exceeds available IRQ's");

	return ret;
}

static int saa7231_request_irq(struct saa7231_dev *saa7231)
{
	struct pci_dev *pdev		= saa7231->pdev;

	int i, ret = 0;

	if (saa7231->int_type == MODE_MSI) {
		dprintk(SAA7231_DEBUG, 1, "Using MSI mode");
		ret = saa7231_enable_msi(saa7231);
	} else if (saa7231->int_type == MODE_MSI_X) {
		dprintk(SAA7231_DEBUG, 1, "Using MSI-X mode");
		ret = saa7231_enable_msix(saa7231);
	}

	if (ret) {
		dprintk(SAA7231_ERROR, 1, "INT-A Mode");
		saa7231->int_type = MODE_INTA;
	}

	if (saa7231->int_type == MODE_MSI) {
		ret = request_irq(pdev->irq,
				  saa7231_msi_handler,
				  IRQF_SHARED,
				  DRIVER_NAME,
				  saa7231);

		if (ret) {
			pci_disable_msi(pdev);
			dprintk(SAA7231_ERROR, 1, "MSI registration failed");
			ret = -EIO;
		}
	}

	if (saa7231->int_type == MODE_MSI_X) {
		for (i = 0; saa7231->msi_vectors_max; i++) {
			ret = request_irq(saa7231->msix_entries[i].vector,
					  saa7231_msix_handler[i].handler,
					  IRQF_SHARED,
					  saa7231_msix_handler[i].desc,
					  saa7231);

			dprintk(SAA7231_ERROR, 1, "%s @ 0x%p", saa7231_msix_handler[i].desc, saa7231_msix_handler[i].handler);
			if (ret) {
				dprintk(SAA7231_ERROR, 1, "%s MSI-X-%d registration failed", saa7231_msix_handler[i].desc, i);
				return -1;
			}
		}
	}

	if (saa7231->int_type == MODE_INTA) {
		ret = request_irq(pdev->irq,
				  saa7231->config->irq_handler,
				  IRQF_SHARED,
				  DRIVER_NAME,
				  saa7231);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "SAA716x IRQ registration failed <%d>", ret);
			ret = -ENODEV;
		}
	}

	return ret;
}

static void saa7231_free_irq(struct saa7231_dev *saa7231)
{
	struct pci_dev *pdev = saa7231->pdev;
	int i, vector;

	if (saa7231->int_type == MODE_MSI_X) {

		for (i = 0; i < saa7231->msi_vectors_max; i++) {
			vector = saa7231->msix_entries[i].vector;
			free_irq(vector, saa7231);
		}

		pci_disable_msix(pdev);

	} else {
		free_irq(pdev->irq, saa7231);
		if (saa7231->int_type == MODE_MSI)
			pci_disable_msi(pdev);
	}
}

#define PURUS_V1A			0x20070926
#define PURUS_V1B			0x20080226

static int saa7231_get_version(struct saa7231_dev *saa7231)
{
	u32 reg;
	int pci = 0;

	reg = SAA7231_RD(SAA7231_BAR0, IF_REGS, IF_REG_PCI_GLOBAL_CONFIG);
	if (reg & 0x200)
		pci = 1;
	else
		pci = 0;

	reg = SAA7231_RD(SAA7231_BAR0, IF_REGS, IF_REG_SPARE);
	if (reg & PURUS_V1A)
		saa7231->version = 0;
	else
		saa7231->version = 1;

	dprintk(SAA7231_INFO, 1, "SAA7231 %s %s found",
		((pci == 1) ? "PCI":"PCI Express"),
		((saa7231->version == 1) ? "V1B":"V1A"));

	return 0;
}

int saa7231_pci_init(struct saa7231_dev *saa7231)
{
	struct pci_dev *pdev		= saa7231->pdev;
	struct saa7231_config *config	= saa7231->config;
	struct card_desc *desc		= config->desc;

	int err = 0, ret = -ENODEV, i, pm_cap;
	u32 msi_cap, offset;
	u8 revision;

	dprintk(SAA7231_ERROR, 1, "Loading %s ver %s ..", DRIVER_NAME, saa7231->ver);
	dprintk(SAA7231_ERROR, 1, "found a %s %s %s device",
		desc->vendor,
		desc->product,
		desc->type);

	mutex_init(&saa7231->dev_lock);

	err = pci_enable_device(pdev);
	if (err) {
		ret = -ENODEV;
		dprintk(SAA7231_ERROR, 1, "ERROR: PCI enable failed (%i)", err);
		goto fail0;
	}

	if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(64))) {
		err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64));
		if (err) {
			dprintk(SAA7231_ERROR, 1, "Unable to obtain 64bit DMA");
			goto fail1;
		}
	} else if ((err = pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32))) != 0) {
		dprintk(SAA7231_ERROR, 1, "Unable to obtain 32bit DMA");
		goto fail1;
	}

	pci_set_master(pdev);

	pm_cap = pci_find_capability(pdev, PCI_CAP_ID_PM);
	if (!pm_cap) {
		dprintk(SAA7231_ERROR, 1, "Cannot find Power Management Capability");
		err = -EIO;
		goto fail1;
	}

	pci_read_config_dword(pdev, 0x10, &offset);
	saa7231->offst_bar0 = offset - (unsigned long) pci_resource_start(pdev, BAR_0);
	pci_read_config_dword(pdev, 0x18, &offset);
	saa7231->offst_bar2 = offset - (unsigned long) pci_resource_start(pdev, BAR_2);
	dprintk(SAA7231_ERROR, 1, "BAR 0 Offset: %02x BAR 2 Offset: %02x",
		saa7231->offst_bar0,
		saa7231->offst_bar2);

	for (i = 0; i < 6; i++) {
		if (pci_resource_flags(pdev, i) & IORESOURCE_MEM) {
			dprintk(SAA7231_ERROR, 1, "BAR%d Start=%lx length=%luM",
				i,
				(unsigned long) pci_resource_start(pdev, i),
				(unsigned long) pci_resource_len(pdev, i) / 1048576);
		}
	}

	if (!request_mem_region(pci_resource_start(pdev, BAR_0),
				pci_resource_len(pdev, BAR_0),
				DRIVER_NAME)) {

		dprintk(SAA7231_ERROR, 1, "BAR0 Request failed");
		ret = -ENODEV;
		goto fail1;
	}

	saa7231->mmio1 = ioremap(pci_resource_start(pdev, BAR_0),
				 pci_resource_len(pdev, BAR_0));

	if (!saa7231->mmio1) {
		dprintk(SAA7231_ERROR, 1, "Mem 0 remap failed");
		ret = -ENODEV;
		goto fail2;
	}

	if (!request_mem_region(pci_resource_start(pdev, BAR_2),
				pci_resource_len(pdev, BAR_2),
				DRIVER_NAME)) {

		dprintk(SAA7231_ERROR, 1, "BAR2 Request failed");
		ret = -ENODEV;
		goto fail3;
	}

	saa7231->mmio2 = ioremap(pci_resource_start(pdev, BAR_2),
				 pci_resource_len(pdev, BAR_2));

	if (!saa7231->mmio1) {
		dprintk(SAA7231_ERROR, 1, "Mem 2 remap failed");
		ret = -ENODEV;
		goto fail4;
	}

	pci_read_config_byte(pdev, PCI_CLASS_REVISION, &revision);

	saa7231->version		=  revision;
	saa7231->msi_vectors_max	= (1 << ((msi_cap >> 17) & 0x07));

	dprintk(SAA7231_ERROR, 0, "    %s [%04x:%04x], ",
		SAA7231_TYPE(revision),
		saa7231->pdev->subsystem_vendor,
		saa7231->pdev->subsystem_device);

	saa7231->caps = SAA7231_CAPS(revision);
	strncpy(saa7231->name, SAA7231_TYPE(revision), 10);

	dprintk(SAA7231_ERROR, 0,
		"irq: %d,\n    mmio(0): 0x%p mmio(2): 0x%p\n",
		saa7231->pdev->irq,
		saa7231->mmio1,
		saa7231->mmio2);

	saa7231->msix_entries = kzalloc(sizeof (struct saa7231_msix_entry) *
					saa7231->msi_vectors_max,
					GFP_KERNEL);

	if (!saa7231->msix_entries) {
		printk(KERN_ERR "saa7231_msi_entries: out of memory\n");
		ret = -ENOMEM;
		goto fail5;
	}

	for (i = 0; i < saa7231->msi_vectors_max; i++)
		saa7231->msix_entries[i].entry = i;

	err = saa7231_request_irq(saa7231);
	if (err < 0) {
		dprintk(SAA7231_ERROR, 1, "SAA7231 IRQ registration failed, err=%d", err);
		ret = -ENODEV;
		goto fail5;
	}

	pci_read_config_dword(pdev, 0x40, &msi_cap);

	dprintk(SAA7231_ERROR, 0, "    SAA%02x %sBit, MSI %s, MSI-X=%d msgs",
		saa7231->pdev->device,
		(((msi_cap >> 23) & 0x01) == 1 ? "64":"32"),
		(((msi_cap >> 16) & 0x01) == 1 ? "Enabled" : "Disabled"),
		saa7231->msi_vectors_max);

	dprintk(SAA7231_ERROR, 0, "\n");

	pci_set_drvdata(pdev, saa7231);
	saa7231_get_version(saa7231);

	return 0;

fail5:
	dprintk(SAA7231_ERROR, 1, "Err: IO Unmap 2");
	if (saa7231->mmio2)
		iounmap(saa7231->mmio2);

fail4:
	dprintk(SAA7231_ERROR, 1, "Err: Release regions 2");
	release_mem_region(pci_resource_start(pdev, BAR_2),
			   pci_resource_len(pdev, BAR_2));

fail3:
	dprintk(SAA7231_ERROR, 1, "Err: IO Unmap 1");
	if (saa7231->mmio1)
		iounmap(saa7231->mmio1);

fail2:
	dprintk(SAA7231_ERROR, 1, "Err: Release regions 1");
	release_mem_region(pci_resource_start(pdev, BAR_0),
			   pci_resource_len(pdev, BAR_0));

fail1:
	dprintk(SAA7231_ERROR, 1, "Err: Disabling device");
	pci_disable_device(pdev);

fail0:
	pci_set_drvdata(pdev, NULL);
	return ret;
}
EXPORT_SYMBOL_GPL(saa7231_pci_init);

void saa7231_pci_exit(struct saa7231_dev *saa7231)
{
	struct pci_dev *pdev = saa7231->pdev;

	saa7231_free_irq(saa7231);

	dprintk(SAA7231_NOTICE, 1, "SAA%02x mem(0): 0x%p mem(2): 0x%p",
		saa7231->pdev->device,
		saa7231->mmio1,
		saa7231->mmio2);

	if (saa7231->mmio2) {
		iounmap(saa7231->mmio2);
		release_mem_region(pci_resource_start(pdev, BAR_2),
				   pci_resource_len(pdev, BAR_2));
	}

	if (saa7231->mmio1) {
		iounmap(saa7231->mmio1);
		release_mem_region(pci_resource_start(pdev, BAR_0),
				   pci_resource_len(pdev, BAR_0));
	}

	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);
}
EXPORT_SYMBOL_GPL(saa7231_pci_exit);

MODULE_DESCRIPTION("SAA7231 bridge driver");
MODULE_AUTHOR("Manu Abraham");
MODULE_LICENSE("GPL");
