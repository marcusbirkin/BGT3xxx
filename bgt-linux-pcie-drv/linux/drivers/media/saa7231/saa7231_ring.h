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

#ifndef __SAA7231_RING_H
#define __SAA7231_RING_H


#define RINGSIZ			16


struct saa7231_ring {
	struct saa7231_dmabuf	*dmabuf;
	struct saa7231_dmabuf	*wcache;
	struct saa7231_dmabuf	*rcache;
	u32			size;
	u32			head;
	u32			tail;
	u32			count;
	spinlock_t		lock;
};

struct saa7231_dmabuf *saa7231_ring_peek(struct saa7231_ring *ring);
struct saa7231_dmabuf *saa7231_ring_read(struct saa7231_ring *ring);
struct saa7231_dmabuf *saa7231_ring_write(struct saa7231_ring *ring);
struct saa7231_dmabuf *saa7231_ring_ilwrite(struct saa7231_ring *ring);
struct saa7231_ring *saa7231_ring_init(struct saa7231_dmabuf *dmabuf, u32 size);
void saa7231_ring_reinit(struct saa7231_ring *ring);
void saa7231_ring_exit(struct saa7231_ring *ring);

#define ring_empty(__ring)			((__ring->count == 0) ? 1 : 0)
#define ring_full(__ring)			((__ring->count >= ring->size) ? 1 : 0)
#define ring_rdpos(__ring)			(__ring->head)
#define ring_wrpos(__ring)			(__ring->tail)
#define ring_peek_buffer(__ring, __index)	(&__ring->dmabuf[__index])
#define ring_count(__ring)			(__ring->count)


#define SAA7231_RING_WRITE(__flags, __ring)	((__flags & STREAM_INTERLACED) ? \
						  saa7231_ring_ilwrite(__ring) : \
						  saa7231_ring_write(__ring))

#define SAA7231_ILBUFS				(VS2D_BUFFERS / 2)

#define SAA7231_VS2DID_WRAP(__flags, __index)	((__flags & STREAM_INTERLACED) ? \
						 (__index %= SAA7231_ILBUFS) : \
						 (__index %= VS2D_BUFFERS))

#define SAA7231_VS2D_BUFFERS(__flags)		((__flags & STREAM_INTERLACED) ? SAA7231_ILBUFS : VS2D_BUFFERS)

#endif /* __SAA7231_RING_H */
