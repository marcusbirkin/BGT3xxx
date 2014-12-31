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

#ifndef __SAA7231_CGU_H
#define __SAA7231_CGU_H

#define PLL160_Q54_M_F72_PROG			15
#define PLL160_Q54_N_F72_PROG			2
#define PLL160_Q54_P_F72_PROG			1
#define PLL160_Q54_direct_F72_PROG		0

#define PLL160_Q54_M_F148_PROG			21
#define PLL160_Q54_N_F148_PROG			3
#define PLL160_Q54_P_F148_PROG			1
#define PLL160_Q54_direct_F148_PROG		1

#define PLL160_Q54_M_F198_PROG			10
#define PLL160_Q54_N_F198_PROG			2
#define PLL160_Q54_P_F198_PROG			0
#define PLL160_Q54_direct_F198_PROG		1

#define PLL160_Q27_M_F72_PROG			0
#define PLL160_Q27_N_F72_PROG			0
#define PLL160_Q27_P_F72_PROG			0
#define PLL160_Q27_direct_F72_PROG		0

#define PLL160_Q27_M_F148_PROG			0
#define PLL160_Q27_N_F148_PROG			0
#define PLL160_Q27_P_F148_PROG			0
#define PLL160_Q27_direct_F148_PROG		0

#define PLL160_Q27_M_F198_PROG			0
#define PLL160_Q27_N_F198_PROG			0
#define PLL160_Q27_P_F198_PROG			0
#define PLL160_Q27_direct_F198_PROG		0

#define PLL550_Q54_M_F99_PROG			511
#define PLL550_Q54_N_F99_PROG			1
#define PLL550_Q54_P_F99_PROG			66

#define PLL550_Q54_M_F96_PROG			10922
#define PLL550_Q54_N_F96_PROG			88
#define PLL550_Q54_P_F96_PROG			66

#define PLL550_Q54_M_F108_PROG			3
#define PLL550_Q54_N_F108_PROG			770
#define PLL550_Q54_P_F108_PROG			66

/* bandwith setup */
#define PLL550_SELR_F99_PROG			0
#define PLL550_SELI_F99_PROG			12
#define PLL550_SELP_F99_PROG			5

#define PLL550_SELR_F96_PROG			0
#define PLL550_SELI_F96_PROG			32
#define PLL550_SELP_F96_PROG			15

#define PLL550_SELR_F108_PROG			0
#define PLL550_SELI_F108_PROG			4
#define PLL550_SELP_F108_PROG			1

#define PLL550_Q27_M_F99_PROG			0
#define PLL550_Q27_N_F99_PROG			0
#define PLL550_Q27_P_F99_PROG			0

#define PLL550_Q27_M_F96_PROG			0
#define PLL550_Q27_N_F96_PROG			0
#define PLL550_Q27_P_F96_PROG			0

#define PLL550_Q27_M_F108_PROG			0
#define PLL550_Q27_N_F108_PROG			0
#define PLL550_Q27_P_F108_PROG			0

#define VADC_SELECT_3_POS 			4
#define VADC_SELECT_2_POS 			2
#define VADC_SELECT_1_POS 			0

#define VADC_3_CLK_SEL_MSK 			0x00000030
#define VADC_2_CLK_SEL_MSK 			0x0000000C
#define VADC_1_CLK_SEL_MSK 			0x00000003



struct pll550_setup {
	u32 M;
	u32 N;
	u32 P;

	u32 sel_r;
	u32 sel_i;
	u32 sel_p;
};

struct pll160_setup {
	u32 M;
	u32 N;
	u32 P;

	u32 direct;
};

enum saa7231_clk_domain {
	CLK_DOMAIN_XTAL_DCSN	= 0,
	CLK_DOMAIN_VADCA_MSCD,
	CLK_DOMAIN_VADCB_MSCD,
	CLK_DOMAIN_TS_MSCD,
	CLK_DOMAIN_MSCD,
	CLK_DOMAIN_MSCD_AXI,
	CLK_DOMAIN_VCP,
	CLK_DOMAIN_REF_PLL_AUD,
	CLK_DOMAIN_128FS_AUD,
	CLK_DOMAIN_128FS_ADC,
	CLK_DOMAIN_EPICS_AUD,
	CLK_DOMAIN_PCC,
	CLK_DOMAIN_CTRL_PCC,
	CLK_DOMAIN_STREAM_PCC,
	CLK_DOMAIN_TSOUT_CAEXT_PCC,

	CLK_DOMAIN_RSV0,
	CLK_DOMAIN_RSV1,
	CLK_DOMAIN_EXT_CA,
	CLK_DOMAIN_128FS_BBADC,
	CLK_DOMAIN_CLOCKS,
};

enum clk_input {
	CLKINPUT_XTAL_DEL	= 0,
	CLKINPUT_XTAL_H,
	CLKINPUT_250M,
	CLKINPUT_PCIe_REFOUT,
	CLKINPUT_TSIN_A,
	CLKINPUT_TSIN_B,
	CLKINPUT_TSIN_CA,
	CLKINPUT_128fs,
	CLKINPUT_PCI,
	CLKINPUT_SPI,
	CLKINPUT_PLL_DEL,
	CLKINPUT_PLL_REFA,
	CLKINPUT_PLL_DSP,
	CLKINPUT_PLL_CDEC,
	CLKINPUT_FDIV_0,
	CLKINPUT_FDIV_1,
	CLKINPUT_FDIV_2,
	CLKINPUT_CLOCKS
};

enum clk_frequency {
	CLK_OFF,
	CLK_LOW,
	CLK_USECASE,
	CLK_NO
};

struct clk_setup {
	u32	clk_in;
	u32	div;
	u32	preset;
};

struct clk_status {
	enum clk_frequency	freq[CLK_DOMAIN_CLOCKS];
	struct clk_setup	setup[CLK_DOMAIN_CLOCKS];
};

struct clk_in {
	u8	active;
	u32	preset;
};

enum clkport {
	CLKPORT_AUDIO = 0,
	CLKPORT_RDS,
	CLKPORT_VIDEO,
	CLKPORT_VBI,
	CLKPORT_DIGITAL0,
	CLKPORT_DIGITAL1,
	CLKPORT_TS0,
	CLKPORT_TS1,
	CLKPORT_TSIN,
	CLKPORT_TSOUT,
	CLKPORTS
};

enum avis_adcmode {
	ADCIN_NONE		= 0x00000000,
	ADCIN_CVBS1		= 0x00000001,
	ADCIN_CVBS2		= 0x00000002,
	ADCIN_CVBS3		= 0x00000004,
	ADCIN_CVBS4		= 0x00000008,
	ADCIN_CVBSATV		= 0x00000010,
	ADCIN_Yc1		= 0x00000020,
	ADCIN_Yc2		= 0x00000040,
	ADCIN_RGB1		= 0x00000080,
	ADCIN_RGB2		= 0x00000100,
	ADCIN_YPbPr1		= 0x00000200,
	ADCIN_YPbPr2		= 0x00000400,
	ADCIN_ATVDIF		= 0x00000800,
	ADCIN_ATVLOWIF		= 0x00001000,
	ADCIN_ATVBB		= 0x00002000,
	ADCIN_DVBTDIF3		= 0x00004000,
	ADCIN_DVBTDIF1		= 0x00008000,
	ADCIN_DVBTLOWIF3	= 0x00010000,
	ADCIN_DVBTLOWIF1	= 0x00020000,
	ADCIN_DVBSZIF		= 0x00040000,
	ADCIN_FMIF		= 0x00080000,
	ADCIN_AUDBB		= 0x00100000,
};

struct port_setup {
	u8	active;
	u8	def;
	enum avis_adcmode last_cfg;
	u32	index;
};

enum saa7231_adc {
	SAA7231_ADC_1		= 0,
	SAA7231_ADC_2,
	SAA7231_ADC_3,
	SAA7231_ADC
};

enum avis_alloc_adc {
	ALLOC_NONE,
	ALLOC_ADC_1,
	ALLOC_ADC_2,
	ALLOC_ADC_3,
};

enum saa7231_mode {
	MODE_NONE		= 0x00,
	AUDIO_CAPTURE		= 0x01,
	VBI_CAPTURE		= 0x02,
	VIDEO_CAPTURE		= 0x04,
	ENCODER_CAPTURE		= 0x08,
	RDS_CAPTURE		= 0x10,
	DIGITAL_CAPTURE		= 0x20,
	DIGITAL_RENDER		= 0x40,
	REMOTE_CONTROL		= 0x80
};

struct saa7231_cgu {
	const struct pll160_setup	*pll160;
	const struct pll550_setup	*pll550;

	u32				adc_in[SAA7231_ADC];
	struct clk_in			clk_in[CLKINPUT_CLOCKS];
	struct clk_status		clk_status;
	u32				clkin_setup;
	u32				clkout_setup;
	struct port_setup		port_setup[CLKPORTS + 1];
};

extern int saa7231_cgu_init(struct saa7231_dev *saa7231);
extern void saa7231_cgu_exit(struct saa7231_dev *saa7231);

extern int saa7231_activate_clocks(struct saa7231_dev *saa7231,
				   enum saa7231_mode mode,
				   enum saa7231_stream_port);

extern int saa7231_disable_clocks(struct saa7231_dev *saa7231,
				  enum saa7231_mode mode,
				  enum saa7231_stream_port port);

#endif /* __SAA7231_CGU_H */
