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
#include <linux/pci.h>
#include <linux/scatterlist.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include "saa7231_priv.h"
#include "saa7231_dmabuf.h"

#include <linux/vmalloc.h>


static int saa7231_allocate_ptable(struct saa7231_dmabuf *dmabuf)
{
	struct saa7231_dev *saa7231	= dmabuf->saa7231;
	struct pci_dev *pdev		= saa7231->pdev;
	void *virt;
	dma_addr_t phys;

	virt = (void *) __get_free_page(GFP_KERNEL);
	if (!virt) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Out of pages !");
		return -ENOMEM;
	}

	phys = dma_map_single(&pdev->dev,
			      virt,
			      SAA7231_PAGE_SIZE,
			      DMA_FROM_DEVICE);

	if (!phys) {
		dprintk(SAA7231_ERROR, 1, "ERROR: map memory failed !");
		return -ENOMEM;
	}

	BUG_ON((unsigned long) phys % SAA7231_PAGE_SIZE);
	dmabuf->pt_phys = phys;
	dmabuf->pt_virt = virt;
	return 0;
}

static void saa7231_free_ptable(struct saa7231_dmabuf *dmabuf)
{
	struct saa7231_dev *saa7231	= dmabuf->saa7231;
	struct pci_dev *pdev		= saa7231->pdev;
	void *virt			= dmabuf->pt_virt;
	dma_addr_t phys 		= dmabuf->pt_phys;

	BUG_ON(!dmabuf);

	if (phys) {
		dma_unmap_single(&pdev->dev,
				 phys,
				 SAA7231_PAGE_SIZE,
				 DMA_FROM_DEVICE);

		phys = 0;
	}

	if (virt) {
		free_page((unsigned long) virt);
		virt = NULL;
	}
}

static void saa7231_dmabuf_sgfree(struct saa7231_dmabuf *dmabuf)
{
	struct scatterlist *sg_list	= dmabuf->sg_list;
	void *mem			= dmabuf->vmalloc;

	BUG_ON(!sg_list);
	dmabuf->virt = NULL;
	if (mem) {
		if (dmabuf->dma_type == SAA7231_DMABUF_INT)
			vfree(mem);
		mem = NULL;
	}
	if (sg_list) {
		kfree(sg_list);
		sg_list = NULL;
	}
}

static int saa7231_dmabuf_sgalloc(struct saa7231_dmabuf *dmabuf, void *buf, int pages)
{
	struct saa7231_dev *saa7231	= dmabuf->saa7231;
	struct scatterlist *sg_list;
	struct page *pg;
	void *vma = NULL, *mem = NULL;
	int i;

	BUG_ON(!pages);
	BUG_ON(!dmabuf);

	sg_list = kzalloc(sizeof (struct scatterlist) * pages , GFP_KERNEL);
	if (!sg_list) {
		dprintk(SAA7231_ERROR, 1, "Failed to allocate memory for scatterlist.");
		return -ENOMEM;
	}
	sg_init_table(sg_list, pages);

	if (!buf) {
		mem = vmalloc((pages + 1) * SAA7231_PAGE_SIZE);
		if (!mem) {
			dprintk(SAA7231_ERROR, 1, "ERROR: vmalloc failed allocating %d pages", pages);
			return -ENOMEM;
		}
		vma = (void *) PAGE_ALIGN (((unsigned long) mem));
		BUG_ON(((unsigned long) vma) % SAA7231_PAGE_SIZE);
	} else {
		dprintk(SAA7231_DEBUG, 1, "DEBUG: Request to add %d pages to SG list", pages);
		vma = buf;
	}
	for (i = 0; i < pages; i++) {
		if (!buf)
			pg = vmalloc_to_page(vma + i * SAA7231_PAGE_SIZE);
		else
			pg = virt_to_page(vma + i * SAA7231_PAGE_SIZE);

		BUG_ON(!pg);
		sg_set_page(&sg_list[i], pg, SAA7231_PAGE_SIZE, 0);
	}
	dmabuf->sg_list	= sg_list;
	dmabuf->virt	= vma;
	dmabuf->vmalloc	= mem;
	dmabuf->pages	= pages;
	return 0;
}

static void saa7231_dmabuf_sgpagefill(struct saa7231_dmabuf *dmabuf,
				      struct scatterlist *sg_list,
				      int pages,
				      int offset)
{
	struct scatterlist *sg_cur;

	u32 *page;
	int i, j, k = 0;
	dma_addr_t addr = 0;

	BUG_ON(!dmabuf);
	BUG_ON(!sg_list);
	BUG_ON(!pages);

	page = dmabuf->pt_virt;

	for (i = 0; i < pages; i++) {
		sg_cur = &sg_list[i];

		if (i == 0)
			dmabuf->offset = (sg_cur->length + sg_cur->offset) % SAA7231_PAGE_SIZE;
		else
			BUG_ON(sg_cur->offset != 0);

		for (j = 0; (j * SAA7231_PAGE_SIZE) < sg_dma_len(sg_cur); j++) {

			if ((offset + sg_cur->offset) >= SAA7231_PAGE_SIZE) {
				offset -= SAA7231_PAGE_SIZE;
				continue;
			}
			addr = ((u64 )sg_dma_address(sg_cur)) + (j * SAA7231_PAGE_SIZE) - sg_cur->offset;
			BUG_ON(!addr);
			page[k * 2 + 0] = ADDR_LSB(addr);
			page[k * 2 + 1] = ADDR_MSB(addr);

			BUG_ON(page[k * 2] % SAA7231_PAGE_SIZE);
			k++;
		}
	}
	for (; k < (SAA7231_PAGE_SIZE / 8); k++) {
		page[k * 2 + 0] = ADDR_LSB(addr);
		page[k * 2 + 1] = ADDR_MSB(addr);
	}
}

int saa7231_dmabuf_alloc(struct saa7231_dev *saa7231,
			 struct saa7231_dmabuf *dmabuf,
			 int size)
{
	struct pci_dev *pdev	= saa7231->pdev;

	int ret;

	BUG_ON(!saa7231);
	BUG_ON(!dmabuf);
	BUG_ON(! (size > 0));

	dmabuf->dma_type = SAA7231_DMABUF_INT;

	dmabuf->vmalloc	 = NULL;
	dmabuf->virt	 = NULL;
	dmabuf->pt_phys	 = 0;
	dmabuf->pt_virt	 = NULL;

	dmabuf->pages	 = 0;
	dmabuf->saa7231	 = saa7231;

	ret = saa7231_allocate_ptable(dmabuf);
	if (ret) {
		dprintk(SAA7231_ERROR, 1, "PT alloc failed, Out of memory");
		goto err1;
	}

	ret = saa7231_dmabuf_sgalloc(dmabuf, NULL, size);
	if (ret) {
		dprintk(SAA7231_ERROR, 1, "Request FAILED! for Virtual contiguous %d byte region with %d %dk pages",
			size * SAA7231_PAGE_SIZE,
			size,
			(SAA7231_PAGE_SIZE / 1024));
		goto err2;
	}

	ret = dma_map_sg(&pdev->dev,
			 dmabuf->sg_list,
			 dmabuf->pages,
			 DMA_FROM_DEVICE);

	if (ret < 0) {
		dprintk(SAA7231_ERROR, 1, "SG map failed, ret=%d", ret);
		goto err3;
	}

	saa7231_dmabuf_sgpagefill(dmabuf, dmabuf->sg_list, ret, 0);

	return 0;
err3:
	saa7231_dmabuf_sgfree(dmabuf);
err2:
	saa7231_free_ptable(dmabuf);
err1:
	return ret;
}

void saa7231_dmabuf_free(struct saa7231_dmabuf *dmabuf)
{
	struct saa7231_dev *saa7231 = dmabuf->saa7231;
	struct pci_dev *pdev = saa7231->pdev;

	BUG_ON(!dmabuf);

	if (dmabuf->sg_list) {

		if (dmabuf->dma_type != SAA7231_DMABUF_EXT_SG) {
			dma_unmap_sg(&pdev->dev,
				     dmabuf->sg_list,
				     dmabuf->pages,
				     DMA_FROM_DEVICE);
		} else {
			dprintk(SAA7231_DEBUG, 1, "INFO: Free external allocation!");
			dmabuf->sg_list = NULL;
		}
	}

	if (dmabuf->dma_type != SAA7231_DMABUF_EXT_SG) {
		saa7231_dmabuf_sgfree(dmabuf);
	} else {
		dmabuf->sg_list = NULL;
		dmabuf->pages = 0;
	}
	saa7231_free_ptable(dmabuf);
}
