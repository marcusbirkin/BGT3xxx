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

#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <linux/spinlock.h>
#include <linux/slab.h>

#include "saa7231_priv.h"
#include "saa7231_dmabuf.h"
#include "saa7231_ring.h"


struct saa7231_dmabuf *saa7231_ring_peek(struct saa7231_ring *ring)
{
	return &ring->dmabuf[ring->head];
}

struct saa7231_dmabuf *saa7231_ring_read(struct saa7231_ring *ring)
{
	struct saa7231_dev *saa7231 = ring->dmabuf->saa7231;
	struct saa7231_dmabuf *dmabuf;

	dmabuf = &ring->dmabuf[ring->head];
	dprintk(SAA7231_DEBUG, 0, "        RING(R): Buf:%d Head:%d Count:%d Size:%d\n",
		dmabuf->count,
		ring->head,
		ring->count,
		ring->size);

	ring->head = (ring->head + 1) % ring->size;
	ring->count--;
	return dmabuf;
}

struct saa7231_dmabuf *saa7231_ring_write(struct saa7231_ring *ring)
{
	struct saa7231_dev *saa7231 = ring->dmabuf->saa7231;
	struct saa7231_dmabuf *dmabuf;

	dmabuf = &ring->dmabuf[ring->tail];
	dprintk(SAA7231_DEBUG, 0, "        RING(W): Buf:%d Tail:%d Count:%d Size:%d\n",
		dmabuf->count,
		ring->tail,
		ring->count,
		ring->size);

	ring->tail = (ring->tail + 1) % ring->size;
	if (ring->count < ring->size)
		ring->count++;
	else
		dprintk(SAA7231_ERROR, 1, "FIFO overflow, icache->count:%d size:%d count:%d head:%d tail:%d",
			ring->wcache->count,
			ring->size,
			ring->count,
			ring->head,
			ring->tail);

	return dmabuf;
}

struct saa7231_ring *saa7231_ring_init(struct saa7231_dmabuf *dmabuf, u32 size)
{
	struct saa7231_ring *ring;

	ring = kzalloc(sizeof (struct saa7231_ring), GFP_KERNEL);
	if (!ring)
		return NULL;

	spin_lock_init(&ring->lock);

	ring->dmabuf	= dmabuf;
	ring->size	= size;
	ring->count	= 0;
	ring->head	= 0;
	ring->tail	= 0;
	ring->wcache	= dmabuf;
	ring->rcache	= NULL;
	return ring;
}

void saa7231_ring_reinit(struct saa7231_ring *ring)
{
	ring->head	= 0;
	ring->tail	= 0;
	ring->count	= 0;
	ring->rcache	= NULL;
}

void saa7231_ring_exit(struct saa7231_ring *ring)
{
	kfree(ring);
}
