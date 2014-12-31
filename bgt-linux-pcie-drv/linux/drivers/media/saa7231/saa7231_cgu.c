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
#include <asm/irq.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#include "saa7231_priv.h"

#include "saa7231_cgu_reg.h"
#include "saa7231_rgu_reg.h"
#include "saa7231_str_reg.h"
#include "saa7231_mod.h"
#include "saa7231_cgu.h"



enum pll160_frequency {
	PLL160_148MHz	= 0,
	PLL160_72MHz,
	PLL160_200MHz,
	PLL160_CUSTOM
};

enum pll550_frequency {
	PLL550_96MHz	= 0,
	PLL550_99MHz,
	PLL550_108MHz,
	PLL550_CUSTOM
};

enum pll550_index {
	PLL550_AD,
	PLL550_REFA,
	PLL550,
};

enum pll160_index {
	PLL160_DSP,
	PLL160_CDEC,
	PLL160,
};

enum saa7231_divider {
	DIVIDER_0 = 0,
	DIVIDER_1,
	DIVIDER_2
};

enum clock_type {
	CLKTYPE_DIRECT,
	CLKTYPE_DIVIDER,
	CLKTYPE_PLL550,
	CLKTYPE_PLL160,
};

struct clockin_static {
	enum clock_type	clk;
	u32		index;
};

static const struct pll160_setup pll160_clk54[] = {
	{
		PLL160_Q54_M_F148_PROG,
		PLL160_Q54_N_F148_PROG,
		PLL160_Q54_P_F148_PROG,
		PLL160_Q54_direct_F148_PROG
	}, {
		PLL160_Q54_M_F72_PROG,
		PLL160_Q54_N_F72_PROG,
		PLL160_Q54_P_F72_PROG,
		PLL160_Q54_direct_F72_PROG
	}, {
		PLL160_Q54_M_F198_PROG,
		PLL160_Q54_N_F198_PROG,
		PLL160_Q54_P_F198_PROG,
		PLL160_Q54_direct_F198_PROG
	}
};

static const struct pll160_setup pll160_clk27[] = {
	{
		PLL160_Q27_M_F148_PROG,
		PLL160_Q27_N_F148_PROG,
		PLL160_Q27_P_F148_PROG,
		PLL160_Q27_direct_F148_PROG
	}, {
		PLL160_Q27_M_F72_PROG,
		PLL160_Q27_N_F72_PROG,
		PLL160_Q27_P_F72_PROG,
		PLL160_Q27_direct_F72_PROG
	}, {
		PLL160_Q27_M_F198_PROG,
		PLL160_Q27_N_F198_PROG,
		PLL160_Q27_P_F198_PROG,
		PLL160_Q27_direct_F198_PROG
	}
};

static const struct pll550_setup pll550_clk54[] = {
	{
		PLL550_Q54_M_F96_PROG,
		PLL550_Q54_N_F96_PROG,
		PLL550_Q54_P_F96_PROG,
		PLL550_SELR_F96_PROG,
		PLL550_SELI_F96_PROG,
		PLL550_SELP_F96_PROG
	}, {
		PLL550_Q54_M_F99_PROG,
		PLL550_Q54_N_F99_PROG,
		PLL550_Q54_P_F99_PROG,
		PLL550_SELR_F99_PROG,
		PLL550_SELI_F99_PROG,
		PLL550_SELP_F99_PROG
	}, {
		PLL550_Q54_M_F108_PROG,
		PLL550_Q54_N_F108_PROG,
		PLL550_Q54_P_F108_PROG,
		PLL550_SELR_F108_PROG,
		PLL550_SELI_F108_PROG,
		PLL550_SELP_F108_PROG
	}
};

static const struct pll550_setup pll550_clk27[] = {
	{
		PLL550_Q27_M_F96_PROG,
		PLL550_Q27_N_F96_PROG,
		PLL550_Q27_P_F96_PROG,
		PLL550_SELR_F96_PROG,
		PLL550_SELI_F96_PROG,
		PLL550_SELP_F96_PROG
	}, {
		PLL550_Q27_M_F99_PROG,
		PLL550_Q27_N_F99_PROG,
		PLL550_Q27_P_F99_PROG,
		PLL550_SELR_F99_PROG,
		PLL550_SELI_F99_PROG,
		PLL550_SELP_F99_PROG
	}, {
		PLL550_Q27_M_F108_PROG,
		PLL550_Q27_N_F108_PROG,
		PLL550_Q27_P_F108_PROG,
		PLL550_SELR_F108_PROG,
		PLL550_SELI_F108_PROG,
		PLL550_SELP_F108_PROG
	}
};

static int saa7231_pll550_setup(struct saa7231_dev *saa7231,
				enum pll550_index pll_index,
				u8 enable,
				enum pll550_frequency freq)
{
	u32 M, N, C;
	struct saa7231_cgu *cgu = saa7231->cgu;

	M = (cgu->pll550[freq].sel_r << CGU_550M_M_REG_SELR_POS) |
	    (cgu->pll550[freq].sel_i << CGU_550M_M_REG_SELI_POS) |
	    (cgu->pll550[freq].sel_p << CGU_550M_M_REG_SELP_POS) |
	    (cgu->pll550[freq].M     << CGU_550M_M_REG_MDEC_POS);

	N = (cgu->pll550[freq].N     << CGU_550M_N_REG_NDEC_POS) |
	    (cgu->pll550[freq].P     << CGU_550M_N_REG_PDEC_POS);

	C = (1 << CGU_550M_C_REG_AUTOBLOCK_POS)	|
	    (1 << CGU_550M_C_REG_CLKEN_POS)	|
	    (1 << CGU_550M_C_REG_PREQ_POS)	|
	    (1 << CGU_550M_C_REG_NREQ_POS)	|
	    (1 << CGU_550M_C_REG_MREQ_POS)	|
	    (0 << CGU_550M_C_REG_PD_POS);

	if ((pll_index == PLL550_REFA) && (!enable))
		dprintk(SAA7231_ERROR, 1, "NOT allowed to disable REFAPLL!");

	if (enable) {
		switch (pll_index) {
		case PLL550_AD:
			SAA7231_WR((1 << CGU_550M_C_REG_PD_POS), SAA7231_BAR0, CGU, CGU_ADPLL_550M_C);
			break;
		case PLL550_REFA:
			SAA7231_WR((1 << CGU_550M_C_REG_PD_POS), SAA7231_BAR0, CGU, CGU_REFAPLL_550M_C);
			break;
		default:
			dprintk(SAA7231_ERROR, 1, "Invalid PLL %d", pll_index);
			return -EINVAL;
		}
	} else {
		/* power off */
		C |= ( 1 << CGU_550M_C_REG_PD_POS);
	}

	switch (pll_index) {
	case PLL550_AD:
		SAA7231_WR(M, SAA7231_BAR0, CGU, CGU_ADPLL_550M_M);
		SAA7231_WR(N, SAA7231_BAR0, CGU, CGU_ADPLL_550M_N);
		SAA7231_WR(C, SAA7231_BAR0, CGU, CGU_ADPLL_550M_C);
		break;
	case PLL550_REFA:
		SAA7231_WR(M, SAA7231_BAR0, CGU, CGU_REFAPLL_550M_M);
		SAA7231_WR(N, SAA7231_BAR0, CGU, CGU_REFAPLL_550M_N);
		SAA7231_WR(C, SAA7231_BAR0, CGU, CGU_REFAPLL_550M_C);
		break;
	default:
		dprintk(SAA7231_ERROR, 1, "Invalid PLL %d", pll_index);
		break;
	}
	return 0;
}

static int saa7231_pll160_setup(struct saa7231_dev *saa7231,
				enum pll160_index pll_index,
				u8 enable,
				enum pll160_frequency freq)
{
	u32 C;
	struct saa7231_cgu *cgu = saa7231->cgu;

	C = (1 << CGU_160M_C_REG_AUTOBLOCK_POS)				|
	    (cgu->pll160[freq].direct << CGU_160M_C_REG_DIRECT_POS)	|
	    (cgu->pll160[freq].M      << CGU_160M_C_REG_MSEL_POS)	|
	    (cgu->pll160[freq].N      << CGU_160M_C_REG_NSEL_POS)	|
	    (cgu->pll160[freq].P      << CGU_160M_C_REG_PSEL_POS)	|
	    (0			      << CGU_160M_C_REG_PD_POS);

	if (enable) {
		switch (pll_index) {
		case PLL160_DSP:
			SAA7231_WR(C | (1 << CGU_160M_C_REG_PD_POS), SAA7231_BAR0, CGU, CGU_DSPPLL_160M_C);
			break;
		case PLL160_CDEC:
			SAA7231_WR(C | (1 << CGU_160M_C_REG_PD_POS), SAA7231_BAR0, CGU, CGU_CDECPLL_160M_C);
			break;
		default:
			dprintk(SAA7231_ERROR, 1, "Invalid PLL %d", pll_index);
			return -EINVAL;
		}
	} else {
		/* power off */
		C |= (1  <<  CGU_160M_C_REG_PD_POS);
	}

	switch (pll_index) {
	case PLL160_DSP:
		SAA7231_WR(C, SAA7231_BAR0, CGU, CGU_DSPPLL_160M_C);
		break;
	case PLL160_CDEC:
		SAA7231_WR(C, SAA7231_BAR0, CGU, CGU_CDECPLL_160M_C);
		break;
	default:
		dprintk(SAA7231_ERROR, 1, "Invalid PLL %d", pll_index);
		return -EINVAL;
	}
	return 0;
}

static const struct clk_setup clk_out_setup[CLK_DOMAIN_CLOCKS][4] = {
	{ {  0, 1 }, {  0, 1 }, {  0, 0 }, { 0, 0 } },
	{ { 10, 2 }, { 10, 1 }, {  0, 0 }, { 0, 0 } },
	{ {  0, 1 }, { 10, 1 }, { 10, 2 }, { 0, 0 } },
	{ { 15, 8 }, {  0, 4 }, {  0, 0 }, { 0, 0 } },
	{ { 15, 1 }, { 15, 1 }, {  0, 0 }, { 0, 0 } },
	{ {  0, 1 }, {  0, 1 }, {  0, 0 }, { 0, 0 } },
	{ {  0, 1 }, {  0, 1 }, {  0, 0 }, { 0, 0 } },
	{ { 11, 1 }, { 11, 1 }, {  0, 0 }, { 0, 0 } },
	{ { 18, 1 }, { 18, 1 }, {  0, 0 }, { 0, 0 } },
	{ {  0, 8 }, {  0, 8 }, {  0, 0 }, { 0, 0 } },
	{ { 11, 1 }, { 11, 1 }, {  0, 0 }, { 0, 0 } },
	{ {  0, 1 }, {  0, 1 }, {  0, 0 }, { 0, 0 } },
	{ {  0, 1 }, {  0, 1 }, {  0, 0 }, { 0, 0 } },
	{ { 11, 1 }, { 11, 1 }, {  0, 0 }, { 0, 0 } },
	{ { 12, 1 }, { 12, 1 }, {  0, 0 }, { 0, 0 } },
	{ {  0, 1 }, {  0, 1 }, {  0, 0 }, { 0, 0 } },
	{ {  0, 1 }, {  0, 1 }, {  0, 0 }, { 0, 0 } },
	{ {  0, 1 }, {  0, 1 }, {  0, 0 }, { 0, 0 } },
	{ {  0, 8 }, {  0, 8 }, {  0, 0 }, { 0, 0 } }

};

static int saa7231_set_clk(struct saa7231_dev *saa7231,
			   enum saa7231_clk_domain domain,
			   enum clk_frequency frequency)
{
	u32 div = 0, src = 0, val = 0, ena;
	struct saa7231_cgu *cgu = saa7231->cgu;

	dprintk(SAA7231_DEBUG, 1, "clk_output=%d   clk_frequency=%d", domain,frequency);
	if ((frequency == CLK_OFF)	&&
	    ((domain == CLK_DOMAIN_PCC) ||
	     (domain == CLK_DOMAIN_XTAL_DCSN))) {

		cgu->clk_status.freq[domain] = frequency;
		dprintk(SAA7231_ERROR, 1, "Can't turn OFF system CLOCK");
		return 0;
	}
	if (domain == CLK_DOMAIN_PCC)
		return 0;
	if (frequency != cgu->clk_status.freq[domain]) {
		switch (frequency) {
		case CLK_OFF:
			div = CLOCKOUT_DIVIDER_LOW;
			ena = CLOCK_DISABLE;
			break;
		case CLK_LOW:
			div = CLOCKOUT_DIVIDER_LOW;
			ena = CLOCK_ENABLE;
			break;
		case CLK_USECASE:
			div = clk_out_setup[domain][0].div;
			if (div > 0)
				div--;
			ena = CLOCK_ENABLE;
			break;
		default:
			return -EINVAL;
		}
		src = clk_out_setup[domain][0].clk_in;
		val = (src << OUTCLOCK_SOURCE_POS)	|
		      (div << OUTCLOCK_INDIVIDER_POS)	|
		      (1   << OUTCLOCK_AUTOBLOCK_POS)	|
		      (ena << OUTCLOCK_ENABLE_POS);
	}
	if (domain <= CLK_DOMAIN_TSOUT_CAEXT_PCC)
		SAA7231_WR(val, SAA7231_BAR0, CGU, CLKOUT(domain));
	else
		SAA7231_WR(val, SAA7231_BAR0, CGU, CLKOUT((domain + 8)));

	cgu->clk_status.setup[domain].clk_in	= src;
	cgu->clk_status.setup[domain].div	= div;
	cgu->clk_status.setup[domain].preset	= 0;
	cgu->clk_status.freq[domain]		= frequency;
	return 0;
}

void saa7231_cgu_exit(struct saa7231_dev *saa7231)
{
	struct saa7231_cgu *cgu = saa7231->cgu;
	BUG_ON(!cgu);
	kfree(cgu);
}
EXPORT_SYMBOL(saa7231_cgu_exit);

int saa7231_cgu_init(struct saa7231_dev *saa7231)
{
	u32 i, val;
	struct saa7231_config *cfg = saa7231->config;
	struct saa7231_cgu *cgu;

	dprintk(SAA7231_ERROR, 1, "Initializing CGU");
	cgu = kzalloc(sizeof (struct saa7231_cgu), GFP_KERNEL);
	if (!cgu) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Couldn't allocate CGU");
		return -ENOMEM;
	}
	saa7231->cgu = cgu;

	switch (cfg->root_clk) {
	case CLK_ROOT_27MHz:
		dprintk(SAA7231_INFO, 1, "Using 27MHz RootClock");
		cgu->pll160 = pll160_clk27;
		cgu->pll550 = pll550_clk27;
		break;
	default:
	case CLK_ROOT_54MHz:
		dprintk(SAA7231_INFO, 1, "Using 54MHz RootClock");
		cgu->pll160 = pll160_clk54;
		cgu->pll550 = pll550_clk54;
		break;
	case CLK_ROOT_CUSTOM:
		dprintk(SAA7231_INFO, 1, "Using CUSTOM RootClock");
		break;
	}
	saa7231_pll550_setup(saa7231, PLL550_AD, 1, PLL550_96MHz);
	saa7231_pll550_setup(saa7231, PLL550_REFA, 1, PLL550_108MHz);
	saa7231_pll160_setup(saa7231, PLL160_DSP, 1, PLL160_72MHz);
	saa7231_pll160_setup(saa7231, PLL160_CDEC, 1, PLL160_200MHz);

	dprintk(SAA7231_INFO, 1, "PLL Status CDEC160: %02x REF550: %02x ADPLL: %02x DSP: %02x",
		SAA7231_RD(SAA7231_BAR0, CGU, CGU_CDECPLL_160M_S),
		SAA7231_RD(SAA7231_BAR0, CGU, CGU_REFAPLL_550M_S),
		SAA7231_RD(SAA7231_BAR0, CGU, CGU_ADPLL_550M_S),
		SAA7231_RD(SAA7231_BAR0, CGU, CGU_DSPPLL_160M_S));

	for (i = 0; i < 19; i++)
		saa7231_set_clk(saa7231, i, CLK_USECASE);

	val = CLOCKOUT_DIVIDER_LOW << OUTCLOCK_INDIVIDER_POS |
		CLOCK_DISABLE          << OUTCLOCK_ENABLE_POS;

	SAA7231_WR(val, SAA7231_BAR0, CGU, CGU_CLK_TEST1_OUT);
	SAA7231_WR(val, SAA7231_BAR0, CGU, CGU_CLK_TEST2_OUT);
	SAA7231_WR(val, SAA7231_BAR0, CGU, CGU_CLK_TESTSHELL_OUT);
	SAA7231_WR(val, SAA7231_BAR0, CGU, CGU_CLK_BYPASS_OUT);
	SAA7231_WR(val, SAA7231_BAR0, CGU, CGU_CLK_DBG_0_OUT);
	SAA7231_WR(val, SAA7231_BAR0, CGU, CGU_CLK_DBG_1_OUT);
	SAA7231_WR(val, SAA7231_BAR0, CGU, CGU_CLK_DBG_2_OUT);
	SAA7231_WR(val, SAA7231_BAR0, CGU, CGU_CLK_DBG_3_OUT);

	val = SAA7231_RD(SAA7231_BAR0, CGU, CGU_GCR5_RW_MONITOR_SB_SELECT);
	val &= ~0x1F000000;
	val |=  0x1F000000;
	SAA7231_WR(val, SAA7231_BAR0, CGU, CGU_GCR5_RW_MONITOR_SB_SELECT);

	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_TS0_LOC_CLGATE);
	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_TS1_LOC_CLGATE);
	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_TS0_EXT_CLGATE);
	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_TS1_EXT_CLGATE);
	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_TSCA_EXT_CLGATE);
	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_VIDEO_LOC_CLGATE);
	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_VIDEO_EXT_CLGATE);
	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_LOC_CLGATE);
	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_AUDIO_EXT_CLGATE);
	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_VBI_LOC_CLGATE);
	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_TSOIF0_CLGATE);
	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_TSOIF1_CLGATE);
	SAA7231_WR(0x01, SAA7231_BAR0, STREAM, STREAM_RA_TSMUX_CLGATE);

	msleep(50);
	val = SAA7231_RD(SAA7231_BAR0, RGU, RGU_RESET_ACTIVE0);
	dprintk(SAA7231_DEBUG, 1, "DEBUG: RGU_RESET_ACTIVE0 %02x", val);
	val = ~val;
	SAA7231_WR(val | 0x00780268, SAA7231_BAR0, RGU, RGU_RESET_CTRL0);
	msleep(50);
	SAA7231_WR(val & 0xff87fd97, SAA7231_BAR0, RGU, RGU_RESET_CTRL0);

	SAA7231_WR(0x00, SAA7231_BAR0, RGU, RGU_RESET_CTRL0);

	val = SAA7231_RD(SAA7231_BAR0, RGU, RGU_RESET_ACTIVE1);
	val = ~val;
	SAA7231_WR(val | 0x0000041A, SAA7231_BAR0, RGU, RGU_RESET_CTRL1);
	msleep(50);
	SAA7231_WR(val & 0xFFFFFBE5, SAA7231_BAR0, RGU, RGU_RESET_CTRL1);

	SAA7231_WR(0x1f, SAA7231_BAR0, STREAM, STREAM_RA_CF_THRESHOLD);
	val = SAA7231_RD(SAA7231_BAR0, RGU, RGU_RESET_ACTIVE0);
	dprintk(SAA7231_DEBUG, 1, "DEBUG: RGU_RESET_ACTIVE0 %02x", val);
	val = SAA7231_RD(SAA7231_BAR0, RGU, RGU_RESET_ACTIVE1);
	dprintk(SAA7231_DEBUG, 1, "DEBUG: RGU_RESET_ACTIVE1 %02x", val);

	saa7231_set_clk(saa7231, CLK_DOMAIN_EXT_CA, CLK_OFF);
	return 0;
}
EXPORT_SYMBOL_GPL(saa7231_cgu_init);

static const struct clockin_static input_clktype[CLKINPUT_CLOCKS] = {
	{ CLKTYPE_DIRECT,  0 },
	{ CLKTYPE_DIRECT,  1 },
	{ CLKTYPE_DIRECT,  2 },
	{ CLKTYPE_DIRECT,  3 },
	{ CLKTYPE_DIRECT,  4 },
	{ CLKTYPE_DIRECT,  5 },
	{ CLKTYPE_DIRECT,  6 },
	{ CLKTYPE_DIRECT,  7 },
	{ CLKTYPE_DIRECT,  8 },
	{ CLKTYPE_DIRECT,  9 },
	{ CLKTYPE_PLL550,  0 },
	{ CLKTYPE_PLL550,  1 },
	{ CLKTYPE_PLL160,  0 },
	{ CLKTYPE_PLL160,  1 },
	{ CLKTYPE_DIVIDER, 0 },
	{ CLKTYPE_DIVIDER, 1 },
	{ CLKTYPE_DIVIDER, 2 }
};

static const u32 clkin_presets[CLKINPUT_CLOCKS][6] = {
	{ 0, 0,	1, 1, 1, 1 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 1, 1, 1, 1 },
	{ 0, 0, 1, 1, 1, 1 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 1, 0 },
	{ 0, 0, 1, 1, 1, 1 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 2, 2, 2, 2, 2, 2 },
	{ 1, 1, 0, 0, 0, 1 },
	{ 2, 2, 0, 2, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0 }
};

static int saa7231_divider_setup(struct saa7231_dev *saa7231, enum saa7231_divider div, u32 val)
{
	switch (div) {
	case DIVIDER_0:
		SAA7231_WR(val, SAA7231_BAR0, CGU, CGU_FDIV_0_C);
		break;
	case DIVIDER_1:
		SAA7231_WR(val, SAA7231_BAR0, CGU, CGU_FDIV_1_C);
		break;
	case DIVIDER_2:
		SAA7231_WR(val, SAA7231_BAR0, CGU, CGU_FDIV_2_C);
		break;
	default:
		dprintk(SAA7231_ERROR, 1, "Incorrect PLL selected, divider=0x%02x", div);
		return -EINVAL;
	}
	return 0;
}

static int saa7231_adc_setup(struct saa7231_dev *saa7231, enum saa7231_adc adc, u32 clk, u8 force)
{
	struct saa7231_cgu *cgu = saa7231->cgu;

	u32 reg;

	if (!force) {
		if (cgu->adc_in[adc] == clk)
			return 0;
	}
	reg = SAA7231_RD(SAA7231_BAR0, CGU, CGU_GCR9_RW_ANA_CLOCK_DIV_SEL);

	switch (adc) {
	case SAA7231_ADC_1:
		reg &= ~VADC_1_CLK_SEL_MSK;
		reg |= (clk << VADC_SELECT_1_POS);
		SAA7231_WR(reg, SAA7231_BAR0, CGU, CGU_GCR9_RW_ANA_CLOCK_DIV_SEL);
		break;
	case SAA7231_ADC_2:
		reg &= ~VADC_2_CLK_SEL_MSK ;
		reg |= (clk << VADC_SELECT_2_POS);
		SAA7231_WR(reg, SAA7231_BAR0, CGU, CGU_GCR9_RW_ANA_CLOCK_DIV_SEL);
		break;
	case SAA7231_ADC_3:
		reg &= ~VADC_3_CLK_SEL_MSK;
		reg |= (clk << VADC_SELECT_3_POS);
		SAA7231_WR(reg, SAA7231_BAR0, CGU, CGU_GCR9_RW_ANA_CLOCK_DIV_SEL);
		break;
	default:
		dprintk(SAA7231_ERROR, 1, "ERROR: Invalid ADC, ADC=0x%02x", adc);
		return -EINVAL;
	}
	cgu->adc_in[adc] = clk;
	return 0;
}

#define SAA7231_VERSIONS			2
#define SAA7231_CLOCKS				24

#define ADC_PRESETS				8
#define CLOCK_PRESETS				9

#define CLOCK_DIV_8_8				0x0009350c

static const u32 div_preset[2] = { CLOCK_DIV_8_8, 0 };

static const u32 adc_sel[SAA7231_CLOCKS][SAA7231_VERSIONS] = {
	{ 1, 1 },
	{ 1, 1 },
	{ 2, 2 },
	{ 2, 2 },
	{ 1, 1 },
	{ 1, 1 },
	{ 1, 1 },
	{ 4, 4 },
	{ 4, 4 },
	{ 4, 4 },
	{ 4, 4 },
	{ 1, 1 },
	{ 1, 1 },
	{ 1, 1 },
	{ 5, 3 },
	{ 7, 6 },
	{ 5, 3 },
	{ 7, 6 },
	{ 5, 5 },
	{ 2, 2 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};

static const u32 adc_clksel[SAA7231_ADC][ADC_PRESETS] = {
	{ 8, 2, 8, 8, 2, 8, 0, 0 },
	{ 8, 8, 2, 8, 2, 0, 8, 0 },
	{ 8, 8, 8, 0, 8, 0, 8, 8 }
};

static int saa7231_adc_clksetup(struct saa7231_dev *saa7231, u32 clock)
{
	u32 clk_sel = 0;

	if (SAA7231_CLOCKS <= clock) {
		BUG_ON(1);
		return -EINVAL;
	}
	clk_sel = adc_sel[clock][saa7231->version];

	if (adc_clksel[SAA7231_ADC_1][clk_sel] != 8)
		saa7231_adc_setup(saa7231, SAA7231_ADC_1, adc_clksel[SAA7231_ADC_1][clk_sel], 0);
	if (adc_clksel[SAA7231_ADC_2][clk_sel] != 8)
		saa7231_adc_setup(saa7231, SAA7231_ADC_2, adc_clksel[SAA7231_ADC_2][clk_sel], 0);
	if (adc_clksel[SAA7231_ADC_3][clk_sel] != 8)
		saa7231_adc_setup(saa7231, SAA7231_ADC_3, adc_clksel[SAA7231_ADC_3][clk_sel], 0);

	return 0;
}

static const u32 clkin_sel[CLK_DOMAIN_CLOCKS][CLOCK_PRESETS] = {

	{ 0,	1,	1,	1,	1,	1,	1,	1,	1 },
	{ 0,	0,	0,	0,	0,	0,	0,	0,	0 },
	{ 0,	1,	1,	1,	1,	1,	1,	1,	1 },
	{ 0,	1,	1,	1,	1,	1,	1,	1,	1 },
	{ 0,	0,	0,	0,	0,	0,	0,	0,	0 },
	{ 0,	0,	0,	0,	0,	0,	0,	0,	0 },
	{ 0,	0,	0,	0,	0,	0,	0,	0,	0 },
	{ 0,	0,	0,	0,	1,	0,	0,	0,	0 },
	{ 0,	1,	1,	1,	1,	1,	1,	1,	1 },
	{ 0,	0,	0,	0,	0,	0,	0,	0,	0 },
	{ 0,	0,	0,	1,	0,	0,	1,	1,	1 },
	{ 0,	1,	1,	1,	1,	1,	1,	1,	1 },
	{ 0,	0,	0,	0,	0,	1,	0,	0,	0 },
	{ 0,	0,	0,	1,	0,	0,	1,	1,	1 },
	{ 0,	0,	1,	0,	1,	0,	0,	0,	0 },
	{ 0,	0,	0,	0,	0,	0,	0,	0,	0 },
	{ 0,	0,	0,	0,	0,	0,	0,	0,	0 }
};

static const u32 clk_sel[SAA7231_CLOCKS] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* analog  1..11 */
	2, 2, 2,                         /* ATV    12..14 */
	3, 7, 3, 8, 6,                   /* DVBT/S 15..19 */
	4, 4,                            /* Audio  20..21 */
	5, 5, 5                          /* TS     22..24 */
};

static const u32 clkin_preset_mask[CLOCK_PRESETS] = {
	0x00000,
	0x0090D,
	0x0490D,
	0x0290D,
	0x0498D,
	0x0190D,
	0x02D0D,
	0x0290D,
	0x0290D,
};

static const u32 clkout_preset_mask[CLOCK_PRESETS] = {
	0x01000,
	0x03041,
	0x035C1,
	0x0301E,
	0x43781,
	0x03001,
	0x0301F,
	0x0301B,
	0x0301B,
};

static const u32 clkout_sel[CLK_DOMAIN_CLOCKS][CLOCK_PRESETS] = {
	{ 0,	1,	1,	1,	1,	1,	1,	1,	1 },
	{ 0,	0,	0,	0,	0,	0,	0,	1,	1 },
	{ 0,	0,	0,	3,	0,	0,	2,	3,	0 },
	{ 0,	0,	0,	1,	0,	0,	2,	1,	2 },
	{ 0,	0,	0,	1,	0,	0,	1,	1,	1 },
	{ 0,	0,	0,	0,	0,	0,	0,	0,	0 },
	{ 0,	1,	1,	0,	0,	0,	0,	0,	0 },
	{ 0,	0,	1,	0,	1,	0,	0,	0,	0 },
	{ 0,	0,	1,	0,	1,	0,	0,	0,	0 },
	{ 0,	0,	0,	0,	1,	0,	0,	0,	0 },
	{ 0,	0,	1,	0,	1,	0,	0,	0,	0 },
	{ 0,	0,	0,	0,	0,	0,	0,	0,	0 },
	{ 1,	1,	1,	1,	1,	1,	1,	1,	1 },
	{ 0,	1,	1,	1,	1,	1,	1,	1,	1 },
	{ 0,	0,	0,	0,	0,	1,	0,	0,	0 },
	{ 0,	0,	0,	0,	0,	0,	0,	0,	0 },
	{ 0,	0,	0,	0,	0,	0,	0,	0,	0 },
	{ 0,	0,	0,	0,	0,	0,	0,	0,	0 },
	{ 0,	0,	0,	0,	1,	0,	0,	0,	0 }
};

static int cgu_system_setup(struct saa7231_dev *saa7231, u32 clockindex)
{
	u32 clk_in = 0;
	u32 clkin_preset = 0;
	enum clock_type clk_type;
	struct saa7231_cgu *cgu = saa7231->cgu;

	u32 clk_index = 0;
	u32 preset = 0, preset_sel = 0, value = 0;
	u32 clk_src, div_out;

	preset = clk_sel[clockindex];

	if (clkin_preset_mask[preset] != (cgu->clkin_setup & clkin_preset_mask[preset])) {
		for (clk_in = 0; clk_in < CLKINPUT_CLOCKS; clk_in++) {

			if ((clkin_sel[clk_in][preset] != 0) &&	(cgu->clk_in[clk_in].active)) {
				clkin_preset	= clkin_presets[clk_in][0];
				clk_type	= input_clktype[clk_in].clk;
				clk_index	= input_clktype[clk_in].index;
				switch (clk_type) {
				case CLKTYPE_PLL160:
					saa7231_pll160_setup(saa7231, clk_index, 1, clkin_preset);
					break;
				case CLKTYPE_PLL550:
					saa7231_pll550_setup(saa7231, clk_index, 1, clkin_preset);
					break;
				case CLKTYPE_DIVIDER:
					saa7231_divider_setup(saa7231, clk_index, div_preset[clkin_preset]);
					break;
				default:
					break;
				}
				cgu->clk_in[clk_in].active = 1;
				cgu->clk_in[clk_in].preset = clkin_preset;
			}
		}
		cgu->clkin_setup |= clkin_preset_mask[preset];
	}

	for (clk_in = 0; clk_in < CLK_DOMAIN_CLOCKS; clk_in++) {
		if (!(CLK_DOMAIN_PCC == clk_in		||
		      CLK_DOMAIN_CTRL_PCC  == clk_in	||
		      CLK_DOMAIN_XTAL_DCSN == clk_in)) {

			preset_sel = clkout_sel[clk_in][preset];
			if (preset_sel == 0)
				continue;

			clk_src = clk_out_setup[clk_in][preset_sel - 1].clk_in;
			div_out = clk_out_setup[clk_in][preset_sel - 1].div;

			if (((  (div_out - 1)	  !=  cgu->clk_status.setup[clk_in].div)	||
				(clk_src 	  !=  cgu->clk_status.setup[clk_in].clk_in)	||
				((preset_sel - 1) !=  cgu->clk_status.setup[clk_in].preset))) {

				if (div_out > 0)
					div_out--;

				value = (clk_src << OUTCLOCK_SOURCE_POS)   |
					(div_out << OUTCLOCK_INDIVIDER_POS)|
					(1	 << OUTCLOCK_AUTOBLOCK_POS)|
					(0	 << OUTCLOCK_ENABLE_POS);

				SAA7231_WR(value, SAA7231_BAR0, CGU, CLKOUT(clk_in));

				cgu->clk_status.setup[clk_in].clk_in	= clk_src;
				cgu->clk_status.setup[clk_in].div	= div_out;
				cgu->clk_status.setup[clk_in].preset	= preset_sel - 1;
				cgu->clk_status.freq[clk_in] = CLK_USECASE;
				cgu->clkout_setup |= clkout_preset_mask[preset];

				dprintk(SAA7231_INFO, 1, "preset case [0..8] selected and executed: %d!", preset);
			}
		}
	}
	cgu->clkout_setup |= clkout_preset_mask[preset];
	return 0;
}

static int cgu_update_clk(struct saa7231_dev *saa7231, u32 clockindex)
{
	int ret;

	ret = cgu_system_setup(saa7231, clockindex);
	BUG_ON(ret < 0);
	if (ret < 0) {
		dprintk(SAA7231_ERROR, 1, "set system use case, ret=%d", ret);
		return ret;
	}
	ret = saa7231_adc_clksetup(saa7231, clockindex);
	BUG_ON(ret < 0);
	if (ret < 0) {
		dprintk(SAA7231_ERROR, 1, "ERROR: ADC Clock setup, ret=%d", ret);
		return ret;
	}
	return 0;
}

static int cgu_getclk_instance(struct saa7231_dev *saa7231,
			       enum saa7231_mode mode,
			       enum saa7231_stream_port dmaport,
			       enum clkport *clkport)
{
	int ret = -EINVAL;

	dprintk(SAA7231_DEBUG, 1, "DEBUG: Mode=0x%02x, dmaport=0x%02x",
		mode,
		dmaport);

	switch (mode) {
	case AUDIO_CAPTURE:
		if (dmaport == STREAM_PORT_AS2D_LOCAL) {
			*clkport = CLKPORT_AUDIO;
			ret = 0;
		}
		break;
	case RDS_CAPTURE:
		if (dmaport == 0) {
			*clkport= CLKPORT_RDS;
			ret = 0;
		}
		break;
	case VBI_CAPTURE:
		if (dmaport == STREAM_PORT_DS2D_AVIS) {
			*clkport = CLKPORT_VBI;
			ret = 0;
		}
		break;
	case VIDEO_CAPTURE:
		if (dmaport == STREAM_PORT_VS2D_AVIS) {
			*clkport = CLKPORT_VIDEO;
			ret = 0;
		}
		break;
	case DIGITAL_CAPTURE:
		switch (dmaport) {
		case STREAM_PORT_TS2D_DTV0:
			dprintk(SAA7231_INFO, 1, "INFO: DMA Port (0x%02x) selected for DTV0", dmaport);
			*clkport = CLKPORT_DIGITAL0;
			ret = 0;
			break;
		case STREAM_PORT_TS2D_DTV1:
			dprintk(SAA7231_INFO, 1, "INFO: DMA Port (0x%02x) selected for DTV1", dmaport);
			*clkport = CLKPORT_DIGITAL1;
			ret = 0;
			break;
		case STREAM_PORT_TS2D_EXTERN0:
			dprintk(SAA7231_INFO, 1, "INFO: DMA Port (0x%02x) selected for TS0", dmaport);
			*clkport = CLKPORT_TS0;
			ret = 0;
			break;
		case STREAM_PORT_TS2D_EXTERN1:
			dprintk(SAA7231_INFO, 1, "INFO: DMA Port (0x%02x) selected for TS1", dmaport);
			*clkport = CLKPORT_TS1;
			ret = 0;
			break;
		case STREAM_PORT_TS2D_CAM:
			dprintk(SAA7231_INFO, 1, "INFO: DMA Port (0x%02x) selected for TS IN", dmaport);
			*clkport = CLKPORT_TSIN;
			ret = 0;
			break;
		default:
			dprintk(SAA7231_ERROR, 1, "ERROR: Unknown DMA Port (0x%02x)", dmaport);
			ret = -EINVAL;
			break;
		}
		break;
	case DIGITAL_RENDER:
		if (dmaport == STREAM_PORT_TSOIF_PC) {
			*clkport = CLKPORT_TSOUT;
			ret = 0;
		}
		break;
	case REMOTE_CONTROL:
		if (dmaport == 0) {
			*clkport = CLKPORTS;
			ret = 0;
		}
		break;
	default:
		dprintk(SAA7231_ERROR, 1, "ERROR: Unknown Mode=0x%02x", mode);
		ret = -EINVAL;
		*clkport = CLKPORTS;
		break;
	}
	dprintk(SAA7231_ERROR, 1, "ret=%d", ret);
	return  ret;
}

static const u8 adcuse_ex[ADC_PRESETS][ADC_PRESETS] = {
	{ 1, 1, 1, 1, 1, 1, 1, 1 },
	{ 1, 0, 1, 1, 0, 1, 0, 0 },
	{ 1, 1, 0, 1, 0, 0, 1, 0 },
	{ 1, 1, 1, 0, 1, 0, 1, 0 },
	{ 1, 0, 0, 1, 0, 0, 0, 0 },
	{ 1, 1, 0, 0, 0, 0, 1, 0 },
	{ 1, 0, 1, 1, 0, 1, 0, 0 },
	{ 1, 0, 0, 0, 0, 0, 0, 0 }
};

#define CLKUSE_FLAG_AUDIO			0x00000001
#define CLKUSE_FLAG_RDS				0x00000002
#define CLKUSE_FLAG_VIDEO			0x00000004
#define CLKUSE_FLAG_VBI				0x00000008
#define CLKUSE_FLAG_DIGITAL_0			0x00000010
#define CLKUSE_FLAG_DIGITAL_1			0x00000020


static int cgu_clkupdate_instance(struct saa7231_dev *saa7231, enum clkport port, u32 index)
{
	u32 clk_sel = 0, mask = 0, i = 0, compare = 0;
	struct saa7231_cgu *cgu = saa7231->cgu;

	if (index >= SAA7231_CLOCKS) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Invalid clock request!");
		return -EINVAL;
	}
	clk_sel = adc_sel[index][saa7231->version];

	switch (port) {
	case CLKPORT_AUDIO:
	case CLKPORT_RDS:
	case CLKPORT_VIDEO:
	case CLKPORT_VBI:
		mask = CLKUSE_FLAG_DIGITAL_0	|
		       CLKUSE_FLAG_DIGITAL_1;
		break;
	case CLKPORT_DIGITAL0:
		mask = CLKUSE_FLAG_AUDIO	|
		       CLKUSE_FLAG_RDS		|
		       CLKUSE_FLAG_VIDEO	|
		       CLKUSE_FLAG_VBI		|
		       CLKUSE_FLAG_DIGITAL_1;
		break;
	case CLKPORT_DIGITAL1:
		mask = CLKUSE_FLAG_AUDIO	|
		       CLKUSE_FLAG_RDS		|
		       CLKUSE_FLAG_VIDEO	|
		       CLKUSE_FLAG_VBI		|
		       CLKUSE_FLAG_DIGITAL_0;
		break;
	default:
		break;
	}
	for (i = CLKPORT_AUDIO; i < CLKPORTS; i++) {
		if (mask & (1 << i) ) {
			if (cgu->port_setup[i].active == 1) {
				compare = adc_sel[cgu->port_setup[i].index][saa7231->version];
				if (adcuse_ex[clk_sel][compare] == 0)
					break;
			}
		}
	}
	return 0;
}

int saa7231_disable_clocks(struct saa7231_dev *saa7231, enum saa7231_mode mode, enum saa7231_stream_port port)
{
	struct saa7231_cgu *cgu		= saa7231->cgu;
	enum clkport clkport		= CLKPORTS;
	enum saa7231_clk_domain clk_out = CLK_DOMAIN_CLOCKS;

	int i, ret = 0, clk_off = 0, analog_active = 0;

	if (((STREAM_PORT_TSOIF_PC	== port)	&&
	     (DIGITAL_RENDER		== mode))	||
	    ((STREAM_PORT_TS2D_CAM	== port)	&&
	     (DIGITAL_CAPTURE		== mode))) {

		clk_off = 1;
	}
	ret = cgu_getclk_instance(saa7231, mode, port, &clkport);
	if (ret) {
		dprintk(SAA7231_ERROR, 1, "ERROR: cgu_getclk_instance failed, ret=%d", ret);
		return -EINVAL;
	}
	if (clkport >= CLKPORTS) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Invalid CLK PORT, clk:%d", clkport);
		return -EINVAL;
	}
	cgu->port_setup[clkport].active = 0;
	if (clk_off) {
		switch (clkport) {
		case CLKPORT_TSIN:
			if (!cgu->port_setup[CLKPORT_TSOUT].active)
				clk_out = CLK_DOMAIN_TSOUT_CAEXT_PCC;
			break;
		case CLKPORT_TSOUT:
			if (!cgu->port_setup[CLKPORT_TSIN].active)
				clk_out = CLK_DOMAIN_TSOUT_CAEXT_PCC;
			break;
		default:
			break;
		}
		if (clk_out < CLK_DOMAIN_CLOCKS)
			saa7231_set_clk(saa7231, clk_out, CLK_OFF);
	}
	for (i = CLKPORT_AUDIO; i < CLKPORT_TS0; i++) {
		if (cgu->port_setup[i].active) {
			if (i == CLKPORT_AUDIO || i == CLKPORT_RDS ||  i == CLKPORT_VIDEO || i == CLKPORT_VBI)
				analog_active = 1;
			continue;
		}
	}
	return  ret;
}

int saa7231_activate_clocks(struct saa7231_dev *saa7231, enum saa7231_mode mode, enum saa7231_stream_port port)
{
	int alt_index, i, ret = 0;
	struct saa7231_cgu *cgu;
	enum avis_adcmode alt_path;
	enum clkport clkport;

	cgu	 = saa7231->cgu;
	alt_path = ADCIN_NONE;
	clkport  = CLKPORTS;
	dprintk(SAA7231_DEBUG, 1, "DEBUG: Activating Clock for Mode=0x%02x, port=0x%02x",
		mode,
		port);

	ret = cgu_getclk_instance(saa7231, mode, port, &clkport);
	BUG_ON(ret < 0);
	if ((!ret) && (clkport < CLKPORTS)) {
		if (cgu->port_setup[clkport].active == 1) {
			dprintk(SAA7231_ERROR, 1, "INFO: noticed active use case!");
			return 0;
		}
		ret = cgu_clkupdate_instance(saa7231, clkport, cgu->port_setup[clkport].index);
		if (ret) {
			switch (cgu->port_setup[clkport].last_cfg) {
			case ADCIN_DVBTLOWIF1:
				alt_path = ADCIN_DVBTLOWIF3;
				break;
			case ADCIN_DVBTLOWIF3:
				alt_path = ADCIN_DVBTLOWIF1;
				break;
			case ADCIN_DVBTDIF1:
				alt_path = ADCIN_DVBTDIF3;
				break;
			case ADCIN_DVBTDIF3:
				alt_path = ADCIN_DVBTDIF1;
				break;
			default:
				dprintk(SAA7231_ERROR, 1, "ERROR: no alternative for  %d clock instance %d!",
					cgu->port_setup[clkport].last_cfg, clkport);
				return ret;
			}
			alt_index = 0;
			for (i = alt_path >> 1; i ; i >>= 1)
				alt_index++;
			ret = cgu_clkupdate_instance(saa7231, clkport, alt_index);
			BUG_ON(ret < 0);
			if (ret) {
				dprintk(SAA7231_ERROR, 1, "ERROR: cgu_clkupdate_instance failed!");
				return  ret;
			} else {
				cgu->port_setup[clkport].last_cfg     = alt_path;
				cgu->port_setup[clkport].index = alt_index;
			}
		}
	}
	if (clkport < CLKPORTS) {
		ret = cgu_update_clk(saa7231, cgu->port_setup[clkport].index);
		BUG_ON(ret < 0);
		if (ret)
			dprintk(SAA7231_ERROR, 1, "ERROR: cgu_update_clk failed!");
		else
			cgu->port_setup[clkport].active = 1;
	} else {
		dprintk(SAA7231_ERROR, 1, "ERROR: Port out of range!, clk=0x%02x", clkport);
	}
	dprintk(SAA7231_INFO, 1, "INFO: activate use case index %d", cgu->port_setup[clkport].index);
	return  ret;
}
EXPORT_SYMBOL_GPL(saa7231_activate_clocks);
