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
#include <linux/delay.h>

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>

#include "dvbdev.h"
#include "dvb_demux.h"
#include "dmxdev.h"
#include "dvb_frontend.h"
#include "dvb_net.h"
#include "dvb_ca_en50221.h"

#include "saa7231_priv.h"
#include "saa7231_dtl.h"
#include "saa7231_cgu.h"
#include "saa7231_msi.h"
#include "saa7231_dmabuf.h"
#include "saa7231_ring.h"
#include "saa7231_stream.h"
#include "saa7231_ring.h"
#include "saa7231_ts2dtl.h"
#include "saa7231_dvb.h"

#include "saa7231_mod.h"
#include "saa7231_s2d_reg.h"

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

#define TS_DEBUG	1
#if 0
static void saa7231_dvb_xfer(unsigned long data)
{
	struct saa7231_dvb *dvb		= (struct saa7231_dvb *) data;
	struct saa7231_stream *stream	= dvb->stream;
	struct saa7231_ring *ring	= stream->ring;
	struct saa7231_dmabuf *dmabuf	= ring->dmabuf;

	dvb_dmx_swfilter_packets(&dvb->demux, dmabuf->virt, 348);
}
#endif
static int saa7231_dvbts2d0_evhandler(struct saa7231_dev *saa7231, int vector)
{
	struct saa7231_dvb *dvb		= &saa7231->dvb[0];
	struct saa7231_stream *stream	= dvb->stream;

	struct saa7231_ring *ring	= stream->ring;
	struct saa7231_dmabuf *dmabuf	= ring->dmabuf;

	int i, index;
	u8 *dbuf;
	unsigned long flags;

	spin_lock_irqsave(&ring->lock, flags);
	if (stream->ops.get_buffer)
		stream->ops.get_buffer(stream, &index, NULL);
	if (!ring_full(ring)) {
		dmabuf = saa7231_ring_write(ring);
		if (stream->ops.set_buffer) {
			if (stream->index_w > TS2D_BUFFERS-1)
				stream->index_w = 0;
			stream->ops.set_buffer(stream, stream->index_w, dmabuf);
			stream->index_w++;
		}
	}
	if (!ring_empty(ring)) {
		dmabuf = saa7231_ring_read(ring);
#if TS_DEBUG
		dbuf = dmabuf->virt;
		for (i = 0; i < 32; i++) {
			if (!(i%32) && !(i == 0))
				dprintk(SAA7231_DEBUG, 0, "\n   ");
			if (!((i%24) || !(i%16) || !(i%8) || !(i%4)))
				dprintk(SAA7231_DEBUG, 0, "  ");
			if (i == 0)
				dprintk(SAA7231_DEBUG, 0, "    ");
			dprintk(SAA7231_DEBUG, 0, "%02x ", dbuf[i]);
		}
#endif
		dvb_dmx_swfilter_packets(&dvb->demux, dmabuf->virt, 348);
	}
	spin_unlock_irqrestore(&ring->lock, flags);
	return 0;
}

static int saa7231_dvbts2d1_evhandler(struct saa7231_dev *saa7231, int vector)
{
	struct saa7231_dvb *dvb		= &saa7231->dvb[1];
	struct saa7231_stream *stream	= dvb->stream;

	struct saa7231_ring *ring	= stream->ring;
	struct saa7231_dmabuf *dmabuf	= ring->dmabuf;

	int i, index;
	u8 *dbuf;
	unsigned long flags;

	spin_lock_irqsave(&ring->lock, flags);
	if (stream->ops.get_buffer)
		stream->ops.get_buffer(stream, &index, NULL);
	if (!ring_full(ring)) {
		dmabuf = saa7231_ring_write(ring);
		if (stream->ops.set_buffer) {
			if (stream->index_w > TS2D_BUFFERS-1)
				stream->index_w = 0;
			stream->ops.set_buffer(stream, stream->index_w, dmabuf);
			stream->index_w++;
		}
	}
	if (!ring_empty(ring)) {
		dmabuf = saa7231_ring_read(ring);
#if TS_DEBUG
		dbuf = dmabuf->virt;
		for (i = 0; i < 32; i++) {
			if (!(i%32) && !(i == 0))
				dprintk(SAA7231_DEBUG, 0, "\n   ");
			if (!((i%24) || !(i%16) || !(i%8) || !(i%4)))
				dprintk(SAA7231_DEBUG, 0, "  ");
			if (i == 0)
				dprintk(SAA7231_DEBUG, 0, "    ");
			dprintk(SAA7231_DEBUG, 0, "%02x ", dbuf[i]);
		}
#endif
		dvb_dmx_swfilter_packets(&dvb->demux, dmabuf->virt, 348);
	}
	spin_unlock_irqrestore(&ring->lock, flags);
	return 0;
}

static struct dvb_vecstr {
	u8 vector;
	u8 stream;
	int (*saa7231_dvb_evhandler) (struct saa7231_dev *saa7231, int vector);
} dvbstr_tbl[] = {
	{ 0x45, 0x2, NULL},
	{ 0x46, 0x3, NULL },
	{ 0x47, 0x0, saa7231_dvbts2d0_evhandler},
	{ 0x48, 0x1, saa7231_dvbts2d1_evhandler},
};

static int saa7231_dma_start(struct saa7231_dvb *dvb)
{
	struct saa7231_dev *saa7231	= dvb->saa7231;
	struct saa7231_stream *stream;
	struct stream_ops *ops;
	int ret;

	if (!dvb->stream) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Adapter:%d No Stream!, exiting ...",
			dvb->adapter);

		ret = -ENOMEM;
		goto exit;
	}

	stream	= dvb->stream;
	ops	= &stream->ops;

	dprintk(SAA7231_DEBUG, 1, " Start DMA Adapter:%d (%s)", dvb->adapter, dvb->name);

	stream->params.bps   = 8;
	stream->params.spl   = 188;
	stream->params.lines = 348;
	stream->params.pitch = 188;
	stream->params.thrsh = 0;
	stream->params.flags = 0;
	stream->params.type  = STREAM_TS;

	if (ops->acquire) {
		ret = ops->acquire(stream);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Stream acquire, Adapter:%d, error=%d",
				dvb->adapter,
				ret);

			ret = -EIO;
			goto exit;
		} else {
			dprintk(SAA7231_INFO, 1, "INFO: Stream acquired, Adapter:%d", dvb->adapter);
		}
	} else {
		ret = -EINVAL;
	}
	if (ops->run) {
		ret = ops->run(stream);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Stream start, Adapter:%d, error=%d",
				dvb->adapter,
				ret);

			ret = -EIO;
			goto exit;
		} else {
			dprintk(SAA7231_INFO, 1, "INFO: Stream started, Adapter:%d", dvb->adapter);
		}
	} else {
		ret = -EINVAL;
	}
exit:
	return ret;
}

static int saa7231_dma_stop(struct saa7231_dvb *dvb)
{
	struct saa7231_dev *saa7231	= dvb->saa7231;
	struct saa7231_stream *stream;
	struct stream_ops *ops;
	int ret;

	if (!dvb->stream) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Adapter:%d No stream! exiting ...",
			dvb->adapter);

		ret = -ENOMEM;
		goto exit;
	}

	stream = dvb->stream;
	ops    = &stream->ops;

	dprintk(SAA7231_DEBUG, 1, "Stop DMA Adapter:%d (%s)", dvb->adapter, dvb->name);

	if (ops->stop) {
		ret = ops->stop(stream);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Stream stop, Adapter:%d, ret=%d",
				dvb->adapter, ret);

			ret = -EIO;
			goto exit;
		} else {
			dprintk(SAA7231_INFO, 1, "INFO: Stream Stopped, Adapter:%d", dvb->adapter);
			ret = 0;
			goto exit;
		}
	} else {
		ret = -EINVAL;
	}
exit:
	return ret;
}

static int saa7231_dvb_start_feed(struct dvb_demux_feed *dvbdmxfeed)
{
	int ret;

	struct dvb_demux *dvbdmx		= dvbdmxfeed->demux;
	struct saa7231_dvb *dvb			= dvbdmx->priv;
	struct saa7231_dev *saa7231		= dvb->saa7231;

	if (!dvbdmx->dmx.frontend) {
		dprintk(SAA7231_DEBUG, 1, "no frontend ?");
		ret = -EINVAL;
		goto exit;
	}
	mutex_lock(&dvb->feedlock);
	dvb->feeds++;
	dprintk(SAA7231_DEBUG, 1, "SAA7231 start feed, feeds=%d", dvb->feeds);

	if (dvb->feeds == 1) {
		dprintk(SAA7231_DEBUG, 1, "SAA7231 start feed & dma");
		ret = saa7231_dma_start(dvb);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "ERROR: DMA START, ret=%d", ret);
			ret = -EINVAL;
			goto err;
		}
#if 0
		tasklet_enable(&dvb->tasklet);
#endif
	}
err:
	mutex_unlock(&dvb->feedlock);
	ret = dvb->feeds;
exit:
	return ret;
}

static int saa7231_dvb_stop_feed(struct dvb_demux_feed *dvbdmxfeed)
{
	int ret;

	struct dvb_demux *dvbdmx		= dvbdmxfeed->demux;
	struct saa7231_dvb *dvb			= dvbdmx->priv;
	struct saa7231_dev *saa7231		= dvb->saa7231;

	if (!dvbdmx->dmx.frontend) {
		dprintk(SAA7231_DEBUG, 1, "no frontend ?");
		ret = -EINVAL;
		goto exit;
	}
	mutex_lock(&dvb->feedlock);
	dvb->feeds--;

	if (!dvb->feeds) {
		dprintk(SAA7231_DEBUG, 1, "saa7231 stop feed and dma");
#if 0
		tasklet_disable(&dvb->tasklet);
#endif
		ret = saa7231_dma_stop(dvb);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "ERROR: DMA STOP, ret=%d", ret);
			ret = -EINVAL;
			goto err;
		}
	}
err:
	mutex_unlock(&dvb->feedlock);
	ret = 0;
exit:
	return ret;
}

#define ADAPTER_0		0
#define ADAPTER_1		1

static char *ext_adap_name[] = {
	[ADAPTER_0] = "SAA7231 DVB External Adapter:1",
	[ADAPTER_1] = "SAA7231 DVB External Adapter:0",
};

static char *int_adap_name[] = {
	[ADAPTER_0] = "SAA7231 DVB-S Internal Adapter",
	[ADAPTER_1] = "SAA7231 DVB-T Internal Adapter",
};

#define ADAPTER_HAS_DVB_S		(saa7231->caps.dvbs)
#define ADAPTER_HAS_DVB_T		(saa7231->caps.dvbt)
#define BUILTIN_ADAPTERS		(saa7231->caps.dvbs + saa7231->caps.dvbt)

#define SAA7231_TSLEN_188		188
#define SAA7231_TSLINES			348

#define TS_PAGES_188			CALC_PAGES(SAA7231_TSLEN_188, SAA7231_TSLINES)

int saa7231_dvb_init(struct saa7231_dev *saa7231)
{
	struct saa7231_config *config	= saa7231->config;
	struct saa7231_stream *stream	= NULL;
	struct saa7231_dvb *dvb 	= NULL;
	enum adapter_type adap_type = 0;
	char ev_name[30];
	int ret, i;
	u8 internal = -1, external = -1;

	saa7231->adapters = 0;

	if (ADAPTER_HAS_DVB_S)
		dprintk(SAA7231_INFO, 1, "INFO: Found internal DVB-S support!");

	if (ADAPTER_HAS_DVB_T)
		dprintk(SAA7231_INFO, 1, "INFO: Found internal DVB-T support!");

	saa7231->adapters = BUILTIN_ADAPTERS + config->ext_dvb_adapters;

	dprintk(SAA7231_INFO, 1, "INFO: Device supoort %d DVB adapters", saa7231->adapters);
	dvb = kzalloc((sizeof (struct saa7231_dvb) * saa7231->adapters), GFP_KERNEL);
	if (!dvb) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Allocating %d DVB Adapters", saa7231->adapters);
		return -ENOMEM;
	}
	saa7231->dvb = dvb;
	if (config->frontend_enable)
		ret = config->frontend_enable(saa7231);

	for (i = 0; i < saa7231->adapters; i++) {

		if (i < BUILTIN_ADAPTERS) {
			dvb->name = int_adap_name[i];
			adap_type = ADAPTER_INT;
			internal++;
		} else {
			dvb->name = ext_adap_name[i - BUILTIN_ADAPTERS];
			adap_type = ADAPTER_EXT;
			external++;
		}

		dprintk(SAA7231_DEBUG, 1, "Registering DVB Adapter: %s", dvb->name);
		ret = dvb_register_adapter(&dvb->dvb_adapter,
					   dvb->name,
					   THIS_MODULE,
					   &saa7231->pdev->dev,
					   adapter_nr);

		if (ret < 0) {

			dprintk(SAA7231_ERROR, 1, "Error registering adapter ERROR=%d", ret);
			return -ENODEV;
		}

		dvb->dvb_adapter.priv		= dvb;
		dvb->demux.dmx.capabilities	= DMX_TS_FILTERING	|
						  DMX_SECTION_FILTERING	|
						  DMX_MEMORY_BASED_FILTERING;

		dvb->demux.priv			= dvb;
		dvb->demux.filternum		= 256;
		dvb->demux.feednum		= 256;
		dvb->demux.start_feed		= saa7231_dvb_start_feed;
		dvb->demux.stop_feed		= saa7231_dvb_stop_feed;
		dvb->demux.write_to_decoder	= NULL;

		dprintk(SAA7231_DEBUG, 1, "dvb_dmx_init");
		ret = dvb_dmx_init(&dvb->demux);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "dvb_dmx_init failed, ERROR=%d", ret);
			goto err0;
		}

		dvb->dmxdev.filternum		= 256;
		dvb->dmxdev.demux		= &dvb->demux.dmx;
		dvb->dmxdev.capabilities	= 0;

		dprintk(SAA7231_DEBUG, 1, "dvb_dmxdev_init");
		ret = dvb_dmxdev_init(&dvb->dmxdev, &dvb->dvb_adapter);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "dvb_dmxdev_init failed, ERROR=%d", ret);
			goto err1;
		}

		dvb->fe_hw.source = DMX_FRONTEND_0;

		ret = dvb->demux.dmx.add_frontend(&dvb->demux.dmx, &dvb->fe_hw);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "dvb_dmx_init failed, ERROR=%d", ret);
			goto err2;
		}

		dvb->fe_mem.source = DMX_MEMORY_FE;

		ret = dvb->demux.dmx.add_frontend(&dvb->demux.dmx, &dvb->fe_mem);
		if (ret) {
			dprintk(SAA7231_ERROR, 1, "dvb_dmx_init failed, ERROR=%d", ret);
			goto err3;
		}

		ret = dvb->demux.dmx.connect_frontend(&dvb->demux.dmx, &dvb->fe_hw);
		if (ret) {
			dprintk(SAA7231_ERROR, 1, "dvb_dmx_init failed, ERROR=%d", ret);
			goto err4;
		}

		ret = dvb_net_init(&dvb->dvb_adapter, &dvb->dvb_net, &dvb->demux.dmx);
		if (ret) {
			dprintk(SAA7231_ERROR, 1, "dvb_net_init failed, ERROR=%d", ret);
		}

		dprintk(SAA7231_DEBUG, 1, "Frontend Init");
		dvb->saa7231 = saa7231;

		if (config->frontend_attach) {
			ret = config->frontend_attach(dvb, i);
			if (ret < 0)
				dprintk(SAA7231_ERROR, 1, "SAA7231 frontend initialization failed");

			if (!dvb->fe) {
				dprintk(SAA7231_ERROR, 1, "A frontend driver was not found for [%04x:%04x] subsystem [%04x:%04x]\n",
					saa7231->pdev->vendor,
					saa7231->pdev->device,
					saa7231->pdev->subsystem_vendor,
					saa7231->pdev->subsystem_device);

			} else {
				ret = dvb_register_frontend(&dvb->dvb_adapter, dvb->fe);
				if (ret < 0) {
					dprintk(SAA7231_ERROR, 1, "BGT7231 register frontend failed");
					goto err4;
				}
			}

		} else {
			dprintk(SAA7231_ERROR, 1, "Frontend attach = NULL");
		}

		stream = saa7231_stream_init(saa7231, DIGITAL_CAPTURE, adap_type, i, TS_PAGES_188);
		if (!stream) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Registering stream for Adapter:%d", i);
			return -ENOMEM;
		}

		dvb->adapter = i;
		dvb->stream = stream;
		dprintk(SAA7231_INFO, 1, "INFO: Registered Stream for Adapter:%d", i);
		mutex_init(&dvb->feedlock);
#if 0
		tasklet_init(&dvb->tasklet, saa7231_dvb_xfer, (unsigned long) dvb);
		tasklet_disable(&dvb->tasklet);
#endif
		if (adap_type == ADAPTER_INT) {
			sprintf(ev_name, "TS2D_DTV%d", internal);
			dvb->vector = 45 + internal;
			dprintk(SAA7231_DEBUG, 1, "INFO: Adding %s Event:%d on Internal adapter:%d",
				ev_name,
				dvb->vector,
				dvb->adapter);

			saa7231_add_irqevent(saa7231,
					     dvb->vector,
					     SAA7231_EDGE_RISING,
					     dvbstr_tbl[dvb->vector - 45].saa7231_dvb_evhandler,
					     ev_name);
		}

		if (adap_type == ADAPTER_EXT) {
			sprintf(ev_name, "TS2D_EXT%d", external);
			dvb->vector = 47 + external;
			dprintk(SAA7231_DEBUG, 1, "INFO: Adding %s Event:%d on External adapter:%d",
				ev_name,
				dvb->vector,
				dvb->adapter);

			saa7231_add_irqevent(saa7231,
					     dvb->vector,
					     SAA7231_EDGE_RISING,
					     dvbstr_tbl[dvb->vector - 45].saa7231_dvb_evhandler,
					     ev_name);

		}
		dvb++;
	}
	return 0;

err4:
	dvb->demux.dmx.remove_frontend(&dvb->demux.dmx, &dvb->fe_mem);
err3:
	dvb->demux.dmx.remove_frontend(&dvb->demux.dmx, &dvb->fe_hw);
err2:
	dvb_dmxdev_release(&dvb->dmxdev);
err1:
	dvb_dmx_release(&dvb->demux);
err0:
	dvb_unregister_adapter(&dvb->dvb_adapter);
	return ret;
}
EXPORT_SYMBOL(saa7231_dvb_init);

void saa7231_dvb_exit(struct saa7231_dev *saa7231)
{
	struct saa7231_dvb	*dvb;
	struct dvb_frontend	*fe;

	struct dvb_adapter	*adapter;
	struct dvb_demux	*demux;
	struct dmxdev		*dmxdev;
	struct dmx_frontend	*fe_hw;
	struct dmx_frontend	*fe_mem;
	struct dvb_net		*dvb_net;

	struct saa7231_stream	*stream;
	int i, vector;

	BUG_ON(!saa7231);
	dvb = saa7231->dvb;

	for (i = 0; i < saa7231->adapters; i++) {
		BUG_ON(!dvb);

		fe	= dvb->fe;
		vector	= dvb->vector;
		demux	= &dvb->demux;
		dmxdev	= &dvb->dmxdev;
		fe_hw	= &dvb->fe_hw;
		fe_mem	= &dvb->fe_mem;
		dvb_net	= &dvb->dvb_net;
		adapter	= &dvb->dvb_adapter;

		stream	= dvb->stream;
		BUG_ON(!stream);

		saa7231_remove_irqevent(saa7231, vector);
		saa7231_stream_exit(stream);

		if (fe) {
			dvb_unregister_frontend(fe);
			dvb_frontend_detach(fe);
		}

		dvb_net_release(dvb_net);
		demux->dmx.remove_frontend(&demux->dmx, fe_mem);
		demux->dmx.remove_frontend(&demux->dmx, fe_hw);
		dvb_dmxdev_release(dmxdev);
		dvb_dmx_release(demux);
		dprintk(SAA7231_DEBUG, 1, "dvb_unregister_adapter %d", i);
		dvb_unregister_adapter(adapter);
		dvb++;
	}
	dvb = saa7231->dvb;
	kfree(dvb);
	return;
}
EXPORT_SYMBOL(saa7231_dvb_exit);
