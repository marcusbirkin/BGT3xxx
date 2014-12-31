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

#ifndef __SAA7231_STREAM_H
#define __SAA7231_STREAM_H

#include "saa7231_ring.h"

enum stream_flags {
	STREAM_FIELD_ODD		= 0x001,
	STREAM_FIELD_EVEN		= 0x002,
	STREAM_INTERLACED		= 0x004,
	STREAM_HD0			= 0x010,
	STREAM_HD1			= 0x020,
	STREAM_PAL			= 0x040,
	STREAM_NTSC			= 0x080,
	STREAM_NOSCALER			= 0x100,
};

enum stream_type {
	STREAM_AUDIO			= 0x01,
	STREAM_VIDEO			= 0x02,
	STREAM_VBI			= 0x04,
	STREAM_TS			= 0x08,
	STREAM_PS			= 0x10
};

struct stream_params {
	u32			bps;
	u32			spl;
	u32     		lines;
	u32			pitch;
	u32			thrsh;
	u32     		ptbls;
	enum stream_flags	flags;
	enum stream_type	type;

	u32			size;
};

struct saa7231_stream;

struct stream_ops {
	int (*acquire)(struct saa7231_stream *stream);
	int (*run)(struct saa7231_stream *stream);
	int (*pause)(struct saa7231_stream *stream);
	int (*stop)(struct saa7231_stream *stream);

	int (*set_buffer)(struct saa7231_stream *stream, u32 index, struct saa7231_dmabuf *dmabuf);
	int (*get_buffer) (struct saa7231_stream *stream, u32 *index, u32 *fid);

	int (*exit)(struct saa7231_stream *stream);
};

struct saa7231_stream {
	enum saa7231_mode		mode;
	enum saa7231_stream_port	port_id;
	enum saa7231_stream_port	port_hw;

	u8				clk;
	u32				config;

	u32				pages;
	u32				frame;

	u8				query;
	u8				req;
	int				vma_refcount;

	wait_queue_head_t		poll;

	struct saa7231_dtl		dtl;
	struct stream_params		params;

	struct stream_ops		ops;

	bool				init;
	spinlock_t			lock;
	int				index_w;

	/* TODO! cleanup, ie; stream->{vdec,adec} */
	struct saa7231_avis		*avis;
	struct avis_audio		*adec;
	struct saa7231_dif		*dif;

	struct saa7231_ring		*ring;
	struct saa7231_dev		*saa7231;
};

extern struct saa7231_stream *saa7231_stream_init(struct saa7231_dev *saa7231,
						  enum saa7231_mode mode,
						  enum adapter_type adap_type,
						  int count,
						  int pages);

extern void saa7231_stream_exit(struct saa7231_stream *stream);

#endif /* __SAA7231_STREAM_H */
