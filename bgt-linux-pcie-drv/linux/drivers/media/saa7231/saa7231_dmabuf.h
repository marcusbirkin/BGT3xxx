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

#ifndef __SAA7231_DMABUF_H
#define __SAA7231_DMABUF_H

#define SAA7231_MAX_BUFFERS	16

#define PTA_LSB(__mem)		((u32 ) (__mem))
#define PTA_MSB(__mem)		((u32 ) ((u64)(__mem) >> 32))

enum saa7231_irq {
	SAA7231_VS2D_AVIS_IRQ		= 37,
	SAA7231_DS2D_AVIS_IRQ		= 41,
	SAA7231_AS2D_LOCAL_IRQ		= 43,
	SAA7231_VS2D_ITU_IRQ		= 39,
	SAA7231_DS2D_ITU_IRQ		= 42,
	SAA7231_AS2D_EXTERN_IRQ		= 44,
	SAA7231_TS2D_DTV0_IRQ		= 45,
	SAA7231_TS2D_DTV1_IRQ		= 46,
	SAA7231_TS2D_EXTERN0_IRQ	= 47,
	SAA7231_TS2D_EXTERN1_IRQ	= 48,
	SAA7231_TS2D_CAM_IRQ		= 49,
	SAA7231_DCSN_PCI_IRQ		= 52,
	SAA7231_TIMER0_IRQ		= 54,
	SAA7231_PL080_DMA_IRQ		= 64,
	SAA7231_TSOIF_PC_IRQ		= 66,
	SAA7231_TSOIF_DCSN_IRQ		= 67,
};

#define SAA7231_PAGE_SIZE		4096

#define ADDR_LSB(__x)			((u64) __x & 0xffffffff)
#define ADDR_MSB(__x)			(((u64) __x >> 32) & 0xffffffff)

#define BUFSIZ(__pages)			(__pages * SAA7231_PAGE_SIZE)

#define CALC_PAGES(__pitch, __lines) 	((__pitch * __lines) % SAA7231_PAGE_SIZE) ?		\
					(((__pitch * __lines) / SAA7231_PAGE_SIZE) + 1) :	\
					((__pitch * __lines) / SAA7231_PAGE_SIZE)

enum saa7231_dma_type {
	SAA7231_DMABUF_EXT_LIN,
	SAA7231_DMABUF_EXT_SG,
	SAA7231_DMABUF_INT
};

enum saa7231_dmabuf_state {
	SAA7231_DMABUF_IDLE		= 0,
	SAA7231_DMABUF_QUEUED		= 1,
	SAA7231_DMABUF_ACTIVE		= 2,
	SAA7231_DMABUF_DONE		= 3,
	SAA7231_DMABUF_ERROR		= 4,
};

struct saa7231_dmabuf {
	enum saa7231_dma_type		dma_type;

	void 				*vmalloc;
	void				*virt;

	dma_addr_t			pt_phys;
	void				*pt_virt;

	struct scatterlist		*sg_list;

	int				pages;
	int				offset;
	int				count;

	enum saa7231_dmabuf_state	state;

	struct saa7231_dev		*saa7231;

};

#define Q_INDEX(__x)			(1 << __x)
#define GET_INDEX(__var, __x)		(__var & Q_INDEX(__x))
#define SET_INDEX(__var, __x)		(__var |= Q_INDEX(__x))

extern void saa7231_dmabuf_free(struct saa7231_dmabuf *dmabuf);
extern int saa7231_dmabuf_alloc(struct saa7231_dev *saa7231,
				struct saa7231_dmabuf *dmabuf,
				int size);

extern int saa7231_dmabuf_clear(struct saa7231_dmabuf *dmabuf);

extern void saa7231_dmabufsync_dev(struct saa7231_dmabuf *dmabuf);
extern void saa7231_dmabufsync_cpu(struct saa7231_dmabuf *dmabuf);


#endif /* __SAA7231_DMABUF_H */
