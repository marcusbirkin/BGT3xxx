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
#include <linux/interrupt.h>

#include "saa7231_mod.h"
#include "saa7231_priv.h"
#include "saa7231_cgu.h"
#include "saa7231_dtl.h"
#include "saa7231_dmabuf.h"
#include "saa7231_stream.h"
#include "saa7231_ts2dtl.h"
//#include "saa7231_dif.h"
//#include "saa7231_vs2dtl.h"
//#include "saa7231_xs2dtl.h"
//#include "saa7231_avis_vid.h"



struct saa7231_stream *saa7231_stream_init(struct saa7231_dev *saa7231,
					   enum saa7231_mode mode,
					   enum adapter_type adap_type,
					   int count,
					   int pages)
{
//	struct saa7231_config *config = saa7231->config;
	struct saa7231_stream *stream;
	struct saa7231_dtl *dtl;
	int ret;

	stream = kzalloc(sizeof(struct saa7231_stream), GFP_KERNEL);
	if (!stream) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Failed allocating streams");
		return NULL;
	}
	stream->mode	= mode;
	stream->saa7231 = saa7231;

	stream->index_w	= 0;
	dtl 		= &stream->dtl;
	spin_lock_init(&stream->lock);
	dprintk(SAA7231_DEBUG, 1, "DEBUG: Initializing Stream with MODE=0x%02x", mode);

	switch (mode) {
	case AUDIO_CAPTURE:
		if (count >= 1) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Invalid Stream port, port=%d", count);
			return NULL;
		}
#if 0
		switch (adap_type) {
		case ADAPTER_INT:
			stream->port_id	= STREAM_PORT_AS2D_LOCAL;
			dtl->module	= AS2D_LOCAL;
			break;
		case ADAPTER_EXT:
			stream->port_id	= STREAM_PORT_AS2D_EXTERN;
			dtl->module	= AS2D_EXTERN;
			break;
		}
		ret = saa7231_xs2dtl_init(stream, pages);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "ERROR: AS2DTL engine initialization failed");
			return NULL;
		}
#endif
		break;
	case VIDEO_CAPTURE:
		if (count >= 1) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Invalid Stream port, port=%d", count);
			return NULL;
		}
#if 0
		switch (adap_type) {
		case ADAPTER_INT:
			stream->port_id	= STREAM_PORT_VS2D_AVIS;
			dtl->module	= VS2D_AVIS;
			if (config->a_tvc) {
				ret = saa7231_dif_attach(stream);
				if (ret < 0) {
					dprintk(SAA7231_ERROR, 1, "ERROR: DIF attach failed, ret=%d", ret);
					return NULL;
				}
			}
			break;
		case ADAPTER_EXT:
			stream->port_id	= STREAM_PORT_VS2D_ITU;
			dtl->module	= VS2D_ITU;
			break;
		}
		ret = saa7231_vs2dtl_init(stream, pages);
		if (ret) {
			dprintk(SAA7231_ERROR, 1, "ERROR: VS2DTL engine initialization failed");
			return NULL;
		}
		ret = saa7231_avis_attach(stream);
		if (ret) {
			dprintk(SAA7231_ERROR, 1, "ERROR: AVIS initialization, attach() failed");
			return NULL;
		}
#endif
		break;
	case VBI_CAPTURE:
		if (count >= 1) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Invalid Stream port, port=%d", count);
			return NULL;
		}

		switch (adap_type) {
		case ADAPTER_INT:
			stream->port_id	= STREAM_PORT_DS2D_AVIS;
			dtl->module	= DS2D_AVIS;
			break;
		case ADAPTER_EXT:
			stream->port_id	= STREAM_PORT_DS2D_ITU;
			dtl->module	= DS2D_ITU;
			break;
		}
#if 0
		ret = saa7231_xs2dtl_init(stream, pages);
		if (ret) {
			dprintk(SAA7231_ERROR, 1, "ERROR: XS2DTL engine initialization failed");
			return NULL;
		}
#endif
		break;
	case DIGITAL_CAPTURE:
		if (count >= 2) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Invalid Stream port, port=%d", count);
			return NULL;
		}

		switch (adap_type) {
		case ADAPTER_INT:
			stream->port_id	= STREAM_PORT_TS2D_DTV0 + count;
			dtl->module	= TS2D0_DTV + (count * 0x1000);
			break;
		case ADAPTER_EXT:
			stream->port_id	= STREAM_PORT_TS2D_EXTERN0 + count;
			dtl->module	= TS2D0_EXTERN + (count * 0x1000);
			break;
		}

		ret = saa7231_ts2dtl_init(stream, pages);
		if (ret) {
			dprintk(SAA7231_ERROR, 1, "ERROR: XS2DTL engine initialization failed");
			return NULL;
		}
		BUG_ON(!stream->ops.acquire);
		stream->params.bps   = 8;
		stream->params.spl   = 188;
		stream->params.lines = 348;
		stream->params.pitch = 188;
		stream->params.thrsh = 0;
		stream->params.flags = 0;
		stream->params.type  = STREAM_TS;
		ret = stream->ops.acquire(stream);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "TS setup failed, ret=%d", ret);
			return NULL;
		}
		break;
	default:
		dprintk(SAA7231_ERROR, 1, "ERROR: Unknown mode=0x%02x", mode);
		BUG_ON(1);
		break;
	}

	dprintk(SAA7231_ERROR, 1, "INFO: Initialized MODE:0x%02x for PORT:%d",
		mode,
		stream->port_id);

	stream->saa7231 = saa7231;
	stream->init = 1;
	return stream;
}
EXPORT_SYMBOL_GPL(saa7231_stream_init);

void saa7231_stream_exit(struct saa7231_stream *stream)
{
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct stream_ops *ops		= &stream->ops;
#if 0
	struct saa7231_config *config	= saa7231->config;
	struct saa7231_dif *difstate	= stream->dif;
	struct saa7231_dif_ops *dif_ops	= &difstate->ops;
	struct saa7231_avis *avis	= stream->avis;
	struct saa7231_avis_ops *decops	= &avis->ops;
	enum saa7231_mode mode		= stream->mode;

	if (mode == VIDEO_CAPTURE) {
		if (stream->port_id == STREAM_PORT_VS2D_AVIS) {
			if (config->a_tvc) {
				BUG_ON(!dif_ops);
				if (dif_ops->dif_detach)
					dif_ops->dif_detach(stream);
			}
			if (avis) {
				BUG_ON(!decops);
				if (decops->detach)
					decops->detach(stream);
			}
		}
	}
#endif
	if (ops->exit) {
		dprintk(SAA7231_INFO, 1, "INFO: Freeing MODE:0x%02x for PORT=0x%02x",
			stream->mode,
			stream->port_id);

		ops->exit(stream);
		kfree(stream);
	} else {
		dprintk(SAA7231_ERROR, 1, "ERROR: Nothing to do");
	}
}
EXPORT_SYMBOL_GPL(saa7231_stream_exit);
