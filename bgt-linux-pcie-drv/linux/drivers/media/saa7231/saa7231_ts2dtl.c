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

#include "saa7231_priv.h"

#include "saa7231_mod.h"
#include "saa7231_mmu_reg.h"
#include "saa7231_s2d_reg.h"
#include "saa7231_cgu.h"
#include "saa7231_cgu_reg.h"
#include "saa7231_msi.h"
#include "saa7231_str_reg.h"

#include "saa7231_dmabuf.h"
#include "saa7231_dtl.h"
#include "saa7231_stream.h"
#include "saa7231_ring.h"
#include "saa7231_ts2dtl.h"


static int saa7231_init_ptables(struct saa7231_stream *stream)
{
	struct saa7231_ring *ring	= stream->ring;
	struct saa7231_dmabuf *dmabuf	= ring->dmabuf;
	struct saa7231_dev *saa7231	= dmabuf->saa7231;

	int port = stream->port_id;
	int ch = DMACH(port);

	BUG_ON(!ring);
	BUG_ON(!dmabuf);
	BUG_ON(!saa7231);

	dprintk(SAA7231_DEBUG, 1, "DEBUG: Initializing PORT:%d DMA_CH:%d with %d Buffers",
		port,
		ch,
		TS2D_BUFFERS);

	SAA7231_WR((TS2D_BUFFERS - 1), SAA7231_BAR0, MMU, MMU_DMA_CONFIG(ch));

	SAA7231_WR(PTA_LSB(dmabuf[0].pt_phys), SAA7231_BAR0, MMU, MMU_PTA0_LSB(ch));
	SAA7231_WR(PTA_MSB(dmabuf[0].pt_phys), SAA7231_BAR0, MMU, MMU_PTA0_MSB(ch));
	SAA7231_WR(PTA_LSB(dmabuf[1].pt_phys), SAA7231_BAR0, MMU, MMU_PTA1_LSB(ch));
	SAA7231_WR(PTA_MSB(dmabuf[1].pt_phys), SAA7231_BAR0, MMU, MMU_PTA1_MSB(ch));
	SAA7231_WR(PTA_LSB(dmabuf[2].pt_phys), SAA7231_BAR0, MMU, MMU_PTA2_LSB(ch));
	SAA7231_WR(PTA_MSB(dmabuf[2].pt_phys), SAA7231_BAR0, MMU, MMU_PTA2_MSB(ch));
	SAA7231_WR(PTA_LSB(dmabuf[3].pt_phys), SAA7231_BAR0, MMU, MMU_PTA3_LSB(ch));
	SAA7231_WR(PTA_MSB(dmabuf[3].pt_phys), SAA7231_BAR0, MMU, MMU_PTA3_MSB(ch));
	SAA7231_WR(PTA_LSB(dmabuf[4].pt_phys), SAA7231_BAR0, MMU, MMU_PTA4_LSB(ch));
	SAA7231_WR(PTA_MSB(dmabuf[4].pt_phys), SAA7231_BAR0, MMU, MMU_PTA4_MSB(ch));
	SAA7231_WR(PTA_LSB(dmabuf[5].pt_phys), SAA7231_BAR0, MMU, MMU_PTA5_LSB(ch));
	SAA7231_WR(PTA_MSB(dmabuf[5].pt_phys), SAA7231_BAR0, MMU, MMU_PTA5_MSB(ch));
	SAA7231_WR(PTA_LSB(dmabuf[6].pt_phys), SAA7231_BAR0, MMU, MMU_PTA6_LSB(ch));
	SAA7231_WR(PTA_MSB(dmabuf[6].pt_phys), SAA7231_BAR0, MMU, MMU_PTA6_MSB(ch));
	SAA7231_WR(PTA_LSB(dmabuf[7].pt_phys), SAA7231_BAR0, MMU, MMU_PTA7_LSB(ch));
	SAA7231_WR(PTA_MSB(dmabuf[7].pt_phys), SAA7231_BAR0, MMU, MMU_PTA7_MSB(ch));

	stream->index_w += 8;
	return 0;
}

static int saa7231_ts2dtl_setparams(struct saa7231_stream *stream)
{
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	struct stream_params *params	= &stream->params;

	struct saa7231_ring *ring	= stream->ring;
	struct saa7231_dmabuf *dmabuf	= ring->dmabuf;

	u32 module			= dtl->module;
	int port			= stream->port_id;
	int id;

	u32 buf_size	= params->pitch * params->lines;
	u32 stride	= 0;
	u32 x_length	= 0;
	u32 y_length	= 0;

	u32 block_control;
	u32 channel_control;

	switch (port) {
	case STREAM_PORT_TS2D_DTV0:
		SAA7231_WR(0x1, SAA7231_BAR0, STREAM, STREAM_RA_TS0_LOC_CLGATE);
		break;
	case STREAM_PORT_TS2D_DTV1:
		SAA7231_WR(0x1, SAA7231_BAR0, STREAM, STREAM_RA_TS1_LOC_CLGATE);
		break;
	case STREAM_PORT_TS2D_EXTERN0:
		SAA7231_WR(stream->clk, SAA7231_BAR0, STREAM, STREAM_RA_TS0_EXT_CLGATE);
		SAA7231_WR((stream->config & 0x7f), SAA7231_BAR0, STREAM, STREAM_RA_PS2P_0_EXT_CFG);
		break;
	case STREAM_PORT_TS2D_EXTERN1:
		SAA7231_WR(stream->clk, SAA7231_BAR0, STREAM, STREAM_RA_TS1_EXT_CLGATE);
		SAA7231_WR((stream->config & 0x7f), SAA7231_BAR0, STREAM, STREAM_RA_PS2P_1_EXT_CFG);
		break;
	case STREAM_PORT_TS2D_CAM:
		SAA7231_WR(0x1, SAA7231_BAR0, STREAM, STREAM_RA_TSCA_EXT_CLGATE);
		SAA7231_WR((stream->config & 0x7f), SAA7231_BAR0, STREAM, STREAM_RA_PS2P_CA_EXT_CFG);
		SAA7231_WR(0x1, SAA7231_BAR0, STREAM, STREAM_RA_TSMUX_CLGATE);
		SAA7231_WR(((stream->config >> 7) & 0x1ff), SAA7231_BAR0, STREAM, STREAM_RA_TS_OUT_CONFIG);
		SAA7231_WR(dtl->cur_input, SAA7231_BAR0, STREAM, STREAM_RA_TS_SEL);
		break;
	default:
		dprintk(SAA7231_ERROR, 1, "Invalid port: 0x%02x", stream->port_id);
		return -EINVAL;
	}

	SAA7231_WR(0x1, SAA7231_BAR0, module, S2D_SW_RST);
	saa7231_init_ptables(stream);
	id = SAA7231_RD(SAA7231_BAR0, module, S2D_MODULE_ID);

	if (params->type != STREAM_TS)
		return -EINVAL;

	stride		= params->pitch;
	x_length	= params->pitch;
	y_length	= params->lines;

	block_control = 0;
	SAA7231_SETFIELD(block_control, S2D_CHx_B0_CTRL_DFOT, 0x1);
	SAA7231_SETFIELD(block_control, S2D_CHx_B0_CTRL_BTAG, 0x0);

	channel_control	= 0x29c;
	if ((id & ~0x00000F00) != 0xA0B11000)
		dprintk(SAA7231_ERROR, 1, "ERROR: TS Module ID: 0x%02x unsupported",id);

	SAA7231_WR(0x0, SAA7231_BAR0, module, S2D_S2D_CTRL);
	SAA7231_WR(0x0, SAA7231_BAR0, module, S2D_CFG_CTRL);
	SAA7231_WR((TS2D_BUFFERS - 1), SAA7231_BAR0, module, S2D_S2D_MAX_B_START);

	SAA7231_WR(0x2, SAA7231_BAR0, module, S2D_CLK_GATING);
	SAA7231_WR(block_control, SAA7231_BAR0, module, S2D_CHx_B0_CTRL(0));
	SAA7231_WR(DMAADDR(0, port, dmabuf, 0), SAA7231_BAR0, module, S2D_CHx_B0_B_START_ADDRESS(0));

	SAA7231_WR(buf_size, SAA7231_BAR0, module, S2D_CHx_B0_B_LENGTH(0));
	SAA7231_WR(stride, SAA7231_BAR0, module, S2D_CHx_B0_STRIDE(0));
	SAA7231_WR(x_length, SAA7231_BAR0, module, S2D_CHx_B0_X_LENGTH(0));
	SAA7231_WR(y_length, SAA7231_BAR0, module, S2D_CHx_B0_Y_LENGTH(0));

	SAA7231_WR(block_control, SAA7231_BAR0, module, S2D_CHx_B1_CTRL(0));
	SAA7231_WR(DMAADDR(0, port, dmabuf, 1), SAA7231_BAR0, module, S2D_CHx_B1_B_START_ADDRESS(0));
	SAA7231_WR(buf_size, SAA7231_BAR0, module, S2D_CHx_B1_B_LENGTH(0));
	SAA7231_WR(stride, SAA7231_BAR0, module, S2D_CHx_B1_STRIDE(0));
	SAA7231_WR(x_length, SAA7231_BAR0, module, S2D_CHx_B1_X_LENGTH(0));
	SAA7231_WR(y_length, SAA7231_BAR0, module, S2D_CHx_B1_Y_LENGTH(0));

	SAA7231_WR(DMAADDR(0, port, dmabuf, 2), SAA7231_BAR0, module,  S2D_CHx_B2_B_START_ADDRESS(0));
	SAA7231_WR(DMAADDR(0, port, dmabuf, 3), SAA7231_BAR0, module,  S2D_CHx_B3_B_START_ADDRESS(0));
	SAA7231_WR(DMAADDR(0, port, dmabuf, 4), SAA7231_BAR0, module,  S2D_CHx_B4_B_START_ADDRESS(0));
	SAA7231_WR(DMAADDR(0, port, dmabuf, 5), SAA7231_BAR0, module,  S2D_CHx_B5_B_START_ADDRESS(0));
	SAA7231_WR(DMAADDR(0, port, dmabuf, 6), SAA7231_BAR0, module,  S2D_CHx_B6_B_START_ADDRESS(0));
	SAA7231_WR(DMAADDR(0, port, dmabuf, 7), SAA7231_BAR0, module,  S2D_CHx_B7_B_START_ADDRESS(0));

	SAA7231_WR(channel_control, SAA7231_BAR0, module, S2D_CHx_CTRL(0));
	return 0;
}

static int saa7231_ts2dtl_run_prepare(struct saa7231_stream *stream)
{
	unsigned long run = 0;
	u32 delay;

	u32 reg;
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	struct saa7231_ring *ring	= stream->ring;
	struct saa7231_dmabuf *dmabuf	= ring->dmabuf;

	u32 module			= dtl->module;
	int port			= stream->port_id;
	int ch				= DMACH(port);

	SAA7231_WR(DMAADDR(0, port, dmabuf, 0), SAA7231_BAR0, module, S2D_CHx_B0_B_START_ADDRESS(0));
	SAA7231_WR(DMAADDR(0, port, dmabuf, 1), SAA7231_BAR0, module, S2D_CHx_B1_B_START_ADDRESS(0));

	reg = SAA7231_RD(SAA7231_BAR0, MMU, MMU_DMA_CONFIG(ch));
	reg &= ~0x40;
	SAA7231_WR(reg, SAA7231_BAR0, MMU, MMU_DMA_CONFIG(ch));
	SAA7231_WR((reg | 0x40), SAA7231_BAR0, MMU, MMU_DMA_CONFIG(ch));

	dtl->addr_prev = 0xffffff;

	delay = 1000;
	do {
		if (!run) {
			msleep(10);
			delay--;
		}
		run = SAA7231_RD(SAA7231_BAR0, MMU, MMU_DMA_CONFIG(ch)) & 0x80;
	} while (!run && delay);

	if (!run) {
		dprintk(SAA7231_ERROR, 1, "ERROR, Preload PTA failed");
		return -EINVAL;
	}
#if 0
	reg = SAA7231_RD(SAA7231_BAR0, module, S2D_CHx_CTRL(0));
	SAA7231_WR(reg | 0x1, SAA7231_BAR0, module, S2D_CHx_CTRL(0));
#endif
	return 0;
}

static int saa7231_ts2dtl_acquire(struct saa7231_stream *stream)
{
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	struct stream_params *params	= &stream->params;
	int ret = 0;

	if (!params->pitch || !params->lines || !params->bps || !params->spl) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Invalid Parameter");
		ret = -EINVAL;
		goto err;
	}

	mutex_lock(&saa7231->dev_lock);
	if (!dtl->clock_run) {
		dprintk(SAA7231_DEBUG, 1, "Activating clock .. mode=0x%02x, port_id=0x%02x",
			stream->mode,
			stream->port_id);

		ret = saa7231_activate_clocks(saa7231, stream->mode, stream->port_id);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "Activating Clock for failed!");
			ret = -EINVAL;
			goto err;
		}
		dtl->clock_run = 1;
	}
	ret = saa7231_ts2dtl_setparams(stream);
	if (ret < 0) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Setup Parameters failed!, err = %d", ret);
		ret = -EINVAL;
		goto err;
	}
	ret = saa7231_ts2dtl_run_prepare(stream);
	if (ret < 0) {
		dprintk(SAA7231_ERROR, 1, "ERROR, prepare to run failed!, err=%d", ret);
		ret = -EINVAL;
		goto err;
	}
err:
	mutex_unlock(&saa7231->dev_lock);
	return 0;
}

static int saa7231_ts2dtl_run(struct saa7231_stream *stream)
{
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	u32 module			= dtl->module;
	u32 reg;

	reg = SAA7231_RD(SAA7231_BAR0, module, S2D_CHx_CTRL(0));
	SAA7231_WR(reg | 0x1, SAA7231_BAR0, module, S2D_CHx_CTRL(0));
	reg = SAA7231_RD(SAA7231_BAR0, module, S2D_S2D_CTRL);

	SAA7231_WR(0x3fff, SAA7231_BAR0, module, S2D_INT_CLR_STATUS(0));
	SAA7231_WR(0x1800, SAA7231_BAR0, module, S2D_INT_SET_ENABLE(0));

	reg |= 0x1;
	SAA7231_WR(reg, SAA7231_BAR0, module, S2D_S2D_CTRL);
	dtl->stream_run = 1;
	return 0;
}

static int saa7231_ts2dtl_pause(struct saa7231_stream *stream)
{
	int ret;
	u32 reg;
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	u32 module			= dtl->module;

	reg = SAA7231_RD(SAA7231_BAR0, module, S2D_S2D_CTRL);

	if (reg & 0x01) {
		SAA7231_WR(reg & 0xfffffffe, SAA7231_BAR0, module, S2D_S2D_CTRL);
		msleep(25);
	}
	dtl->stream_run = 0;

	if (reg & 0x01) {
		reg = SAA7231_RD(SAA7231_BAR0, module, S2D_INT_ENABLE(0));
		SAA7231_WR(reg, SAA7231_BAR0, module, S2D_INT_CLR_ENABLE(0));
		reg = SAA7231_RD(SAA7231_BAR0, module, S2D_INT_STATUS(0));
		SAA7231_WR(reg, SAA7231_BAR0, module, S2D_INT_CLR_STATUS(0));

		ret = saa7231_ts2dtl_setparams(stream);
		if (ret) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Setting parameters, err=%d", ret);
			ret = -EINVAL;
			goto err;
		}
	}
	ret = 0;

	dtl->x_length = 0;
	dtl->y_length = 0;
	dtl->b_length = 0;
	dtl->data_loss = 0;
	dtl->wr_error = 0;
	dtl->tag_error = 0;
	dtl->dtl_halt = 0;
err:
	return ret;
}

static int saa7231_ts2dtl_stop(struct saa7231_stream *stream)
{
	struct saa7231_dtl *dtl = &stream->dtl;

	if (dtl->clock_run)
		dtl->clock_run = 0;

	return 0;
}

static int saa7231_ts2dtl_set_buffer(struct saa7231_stream *stream,
				     u32 index,
				     struct saa7231_dmabuf *dmabuf)
{
	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	u32 module			= dtl->module;

	int port			= stream->port_id;
	int ch				= DMACH(port);
	int ret;

	switch (index) {
	case 0:
		SAA7231_WR(PTA_LSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(LSB, index, ch));
		SAA7231_WR(PTA_MSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(MSB, index, ch));
		SAA7231_WR(DMAADDR(0, port, dmabuf, 0), SAA7231_BAR0, module, S2D_CHx_B0_B_START_ADDRESS(0));
		break;
	case 1:
		SAA7231_WR(PTA_LSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(LSB, index, ch));
		SAA7231_WR(PTA_MSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(MSB, index, ch));
		SAA7231_WR(DMAADDR(0, port, dmabuf, 1), SAA7231_BAR0, module, S2D_CHx_B1_B_START_ADDRESS(0));
		break;
	case 2:
		SAA7231_WR(PTA_LSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(LSB, index, ch));
		SAA7231_WR(PTA_MSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(MSB, index, ch));
		SAA7231_WR(DMAADDR(0, port, dmabuf, 2), SAA7231_BAR0, module, S2D_CHx_B2_B_START_ADDRESS(0));
		break;
	case 3:
		SAA7231_WR(PTA_LSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(LSB, index, ch));
		SAA7231_WR(PTA_MSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(MSB, index, ch));
		SAA7231_WR(DMAADDR(0, port, dmabuf, 3), SAA7231_BAR0, module, S2D_CHx_B3_B_START_ADDRESS(0));
		break;
	case 4:
		SAA7231_WR(PTA_LSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(LSB, index, ch));
		SAA7231_WR(PTA_MSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(MSB, index, ch));
		SAA7231_WR(DMAADDR(0, port, dmabuf, 4), SAA7231_BAR0, module, S2D_CHx_B4_B_START_ADDRESS(0));
		break;
	case 5:
		SAA7231_WR(PTA_LSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(LSB, index, ch));
		SAA7231_WR(PTA_MSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(MSB, index, ch));
		SAA7231_WR(DMAADDR(0, port, dmabuf, 5), SAA7231_BAR0, module, S2D_CHx_B5_B_START_ADDRESS(0));
		break;
	case 6:
		SAA7231_WR(PTA_LSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(LSB, index, ch));
		SAA7231_WR(PTA_MSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(MSB, index, ch));
		SAA7231_WR(DMAADDR(0, port, dmabuf, 6), SAA7231_BAR0, module, S2D_CHx_B6_B_START_ADDRESS(0));
		break;
	case 7:
		SAA7231_WR(PTA_LSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(LSB, index, ch));
		SAA7231_WR(PTA_MSB(dmabuf->pt_phys), SAA7231_BAR0, MMU, MMU_PTA(MSB, index, ch));
		SAA7231_WR(DMAADDR(0, port, dmabuf, 7), SAA7231_BAR0, module, S2D_CHx_B7_B_START_ADDRESS(0));
		break;
	default:
		dprintk(SAA7231_ERROR, 1, "Invalid Index:%d", index);
		ret = -EINVAL;
		goto exit;
	}
exit:
	return ret;
}

static int saa7231_ts2dtl_get_buffer(struct saa7231_stream *stream, u32 *index, u32 *fid)
{
	u32 reg, stat, index_prev, addr_curr;

	struct saa7231_dev *saa7231	= stream->saa7231;
	struct saa7231_dtl *dtl		= &stream->dtl;
	u32 module			= dtl->module;
	int ret;

	if (!dtl->stream_run) {
		*index = 0;
		return 0;
	}
	reg = SAA7231_RD(SAA7231_BAR0, module, S2D_CHx_CURRENT_ADDRESS(0));
	stat = SAA7231_RD(SAA7231_BAR0, module, S2D_INT_STATUS(0));

	if (stat) {
		SAA7231_WR(stat, SAA7231_BAR0, module, S2D_INT_CLR_STATUS(0));
		if (stat & 0x4)
			dtl->x_length++;
		if (stat & 0x8)
			dtl->y_length++;
		if (stat & 0x10)
			dtl->b_length++;
		if (stat & 0x20)
			dtl->data_loss++;
		if (stat & 0x40)
			dtl->wr_error++;
		if (stat & 0x80)
			dtl->tag_error++;
		if (stat & 0x100)
			dtl->dtl_halt++;
	}

	if (reg == 0xffffffff) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Invalid stream: %d", stream->port_id);
		ret = -EINVAL;
		goto exit;
	}
	reg = SAA7231_RD(SAA7231_BAR0, module, S2D_CHx_CURRENT_ADDRESS(0));
	*index =  INDEX(reg);

	index_prev = INDEX(dtl->addr_prev);
	dprintk(SAA7231_DEBUG, 1, "DEBUG: prev:%d index:%d", index_prev, *index);

	if (index_prev == *index) {
		dprintk(SAA7231_ERROR, 1, "ERROR: prev:%d = index:%d", index_prev, *index);
		addr_curr = SAA7231_RD(SAA7231_BAR0, module, S2D_CHx_CURRENT_ADDRESS(0));
		*index = INDEX(addr_curr);
		reg = addr_curr;
	}
	dtl->addr_prev = reg;
	ret = 0;
exit:
	return ret;
}

static int saa7231_ts2dtl_exit(struct saa7231_stream *stream)
{
	struct saa7231_ring *ring	= stream->ring;
	struct saa7231_dmabuf *dmabuf	= ring->dmabuf;
	struct saa7231_dev *saa7231	= dmabuf->saa7231;

	int i;

	dprintk(SAA7231_DEBUG, 1, "Free TS2DTL engine ..");

	for (i = 0; i < RINGSIZ; i++) {
		dmabuf = &ring->dmabuf[i];
		saa7231_dmabuf_free(dmabuf);
	}
	dmabuf = &ring->dmabuf[0];
	kfree(dmabuf);
	saa7231_ring_exit(ring);
	return 0;
}

int saa7231_ts2dtl_init(struct saa7231_stream *stream, int pages)
{
	int i, ret = 0;
	struct stream_ops *ops		= &stream->ops;
	struct saa7231_dev *saa7231	= stream->saa7231;

	struct saa7231_dmabuf *dmabuf;
	struct saa7231_ring *ring;

	dprintk(SAA7231_DEBUG, 1, "TS2DTL engine Initializing .....");
	mutex_lock(&saa7231->dev_lock);

	switch (stream->port_id) {
	case STREAM_PORT_TS2D_DTV0:
	case STREAM_PORT_TS2D_DTV1:
		stream->port_hw = STREAM_TS;
		stream->config   = 0x0;
		break;
	case STREAM_PORT_TS2D_EXTERN0:
		stream->port_hw = STREAM_TS;
		stream->config	= saa7231->config->ts0_cfg;
		stream->clk	= saa7231->config->ts0_clk;
		break;
	case STREAM_PORT_TS2D_EXTERN1:
		stream->port_hw = STREAM_TS;
		stream->config	= saa7231->config->ts1_cfg;
		stream->clk	= saa7231->config->ts1_clk;
		break;
	case STREAM_PORT_TS2D_CAM:
		stream->port_hw = STREAM_TS;
		stream->config	= 0x0181;
		break;
	default:
		dprintk(SAA7231_ERROR, 1, "ERROR: Invalid DMA setup!");
		stream->config	= 0;
		return -EINVAL;
	}
	dmabuf = kzalloc(sizeof (struct saa7231_dmabuf) * RINGSIZ, GFP_KERNEL);
	if (!dmabuf) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Allocating %d TS2D Buffers", RINGSIZ);
		ret = -ENOMEM;
		goto err;
	}
	for (i = 0; i < RINGSIZ; i++) {
		ret = saa7231_dmabuf_alloc(saa7231, &dmabuf[i], pages);
		if (ret < 0) {
			dprintk(SAA7231_ERROR, 1, "ERROR: Allocating DMA buffers, error=%d", ret);
			ret = -ENOMEM;
			goto err;
		}
		dmabuf[i].count = i;
	}
	dprintk(SAA7231_DEBUG, 1, "DEBUG: %d Buffers Allocated with %d pages each.", RINGSIZ, pages);
	ring = saa7231_ring_init(dmabuf, RINGSIZ);
	if (!ring) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Allocating TS2D Ring");
		ret = -ENOMEM;
		goto err;
	}
	stream->ring = ring;
	saa7231_init_ptables(stream);

	switch (stream->port_id) {
	case STREAM_PORT_TS2D_DTV0:
		SAA7231_WR(0, SAA7231_BAR0, STREAM, STREAM_RA_TS0_LOC_CLGATE);
		break;
	case STREAM_PORT_TS2D_DTV1:
		SAA7231_WR(0, SAA7231_BAR0, STREAM, STREAM_RA_TS1_LOC_CLGATE);
		break;
	case STREAM_PORT_TS2D_EXTERN0:
		SAA7231_WR(0, SAA7231_BAR0, STREAM, STREAM_RA_TS0_EXT_CLGATE);
		break;
	case STREAM_PORT_TS2D_EXTERN1:
		SAA7231_WR(0, SAA7231_BAR0, STREAM, STREAM_RA_TS1_EXT_CLGATE);
		break;
	case STREAM_PORT_TS2D_CAM:
		SAA7231_WR(0, SAA7231_BAR0, STREAM, STREAM_RA_TSCA_EXT_CLGATE);
		SAA7231_WR(0, SAA7231_BAR0, STREAM, STREAM_RA_TSMUX_CLGATE);
		break;
	default:
		ret = -EINVAL;
		goto err;
	}

	ops->acquire	= saa7231_ts2dtl_acquire;
	ops->run	= saa7231_ts2dtl_run;
	ops->pause	= saa7231_ts2dtl_pause;
	ops->stop	= saa7231_ts2dtl_stop;
	ops->set_buffer = saa7231_ts2dtl_set_buffer;
	ops->get_buffer = saa7231_ts2dtl_get_buffer;
	ops->exit 	= saa7231_ts2dtl_exit;
err:
	mutex_unlock(&saa7231->dev_lock);
	return ret;
}
