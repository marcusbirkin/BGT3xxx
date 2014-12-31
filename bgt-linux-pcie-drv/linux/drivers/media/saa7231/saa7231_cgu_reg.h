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

#ifndef __SAA7231_CGU_REG_H
#define __SAA7231_CGU_REG_H


#define CLOCKOUT_DIVIDER_LOW			0x07

#define CLOCK_ENABLE				0
#define CLOCK_DISABLE				1
#define CLOCK_ADC_IGNORE			0x8
#define OUTCLOCK_ENABLE_POS			0
#define OUTCLOCK_AUTOBLOCK_POS			11

#define OUTCLOCK_SOURCE_POS			24
#define OUTCLOCK_INDIVIDER_POS			2

#define CGU_RDET_0_REG_CLK_PPRESENT_POS		0
#define CGU_550M_S_REG_FR_AD_PLL_POS		1
#define CGU_550M_S_REG_LOCK_POS			0
#define CGU_550M_C_REG_CLKSRC_POS		24
#define CGU_550M_C_REG_AUTOBLOCK_POS		11
#define CGU_550M_C_REG_PREQ_POS			10
#define CGU_550M_C_REG_NREQ_POS			9
#define CGU_550M_C_REG_MREQ_POS			8
#define CGU_550M_C_REG_CLKEN_POS		4
#define CGU_550M_C_REG_PD_POS			0
#define CGU_550M_M_REG_SELR_POS			28
#define CGU_550M_M_REG_SELI_POS			22
#define CGU_550M_M_REG_SELP_POS			17
#define CGU_550M_M_REG_MDEC_POS			0
#define CGU_550M_N_REG_NDEC_POS			12
#define CGU_550M_N_REG_PDEC_POS			0

#define CGU_160M_C_REG_AUTOBLOCK_POS		11
#define CGU_160M_C_REG_DIRECT_POS		7
#define CGU_160M_C_REG_PSEL_POS			8
#define CGU_160M_C_REG_NSEL_POS			12
#define CGU_160M_C_REG_MSEL_POS			16
#define CGU_160M_C_REG_PD_POS			0


#define CGU_RCD0				0x0000
#define CGU_RCD1				0x0004
#define CGU_RCD2				0x0008
#define CGU_RCD3				0x000C
#define CGU_RFRQ				0x0014
#define CGU_RDET_0				0x0018
#define CGU_ADPLL_550M_S			0x001C
#define CGU_ADPLL_550M_C			0x0020
#define CGU_ADPLL_550M_M			0x0024
#define CGU_ADPLL_550M_N			0x0028
#define CGU_REFAPLL_550M_S			0x002C
#define CGU_REFAPLL_550M_C			0x0030
#define CGU_REFAPLL_550M_M			0x0034
#define CGU_REFAPLL_550M_N			0x0038
#define CGU_DSPPLL_160M_S			0x003C
#define CGU_DSPPLL_160M_C			0x0040
#define CGU_CDECPLL_160M_S			0x0044
#define CGU_CDECPLL_160M_C			0x0048
#define CGU_FDIV_0_C				0x004C
#define CGU_FDIV_1_C				0x0050
#define CGU_FDIV_2_C				0x0054

#define CGU_CLK_XTAL_DCSN_OUT			0x0058
#define CGU_CLK_VADCA_MSCD_OUT			0x005C
#define CGU_CLK_VADCB_MSCD_OUT			0x0060
#define CGU_CLK_TS_MSCD_OUT			0x0064
#define CGU_CLK_MSCD_OUT			0x0068
#define CGU_CLK_MSCD_AXI_OUT			0x006C
#define CGU_CLK_VCP_OUT				0x0070
#define CGU_CLK_REF_PLL_AUD_OUT			0x0074
#define CGU_CLK_128FS_AUD_OUT			0x0078
#define CGU_CLK_128FS_ADC_OUT			0x007C
#define CGU_CLK_EPICS_AUD_OUT			0x0080
#define CGU_CLK_PCC_OUT				0x0084
#define CGU_CLK_CTRL_PCC_OUT			0x0088
#define CGU_CLK_STREAM_PCC_OUT			0x008C
#define CGU_CLK_TS_OUT_CA_EXT_PCC_OUT		0x0090
#define CGU_CLK_TEST1_OUT			0x0094
#define CGU_CLK_TEST2_OUT			0x0098
#define CGU_CLK_TESTSHELL_OUT			0x009C
#define CGU_CLK_BYPASS_OUT			0x00A0
#define CGU_CLK_DBG_0_OUT			0x00A4
#define CGU_CLK_DBG_1_OUT			0x00A8
#define CGU_CLK_DBG_2_OUT			0x00AC
#define CGU_CLK_DBG_3_OUT			0x00B0
#define CGU_CLK_RSV_0_OUT			0x00B4
#define CGU_CLK_RSV_1_OUT			0x00B8
#define CGU_CLK_EXT_CA_OUT			0x00BC

#define CLKOUT_BASE				0x0058
#define CLKOUT(__module) 			(CLKOUT_BASE + (__module * 4))

#define CGU_CLK_128FS_BBADC			0x00C0
#define CGU_GCR0_R				0x0D00
#define CGU_GCR1_R				0x0D04
#define CGU_GCR2_RW_SIST_CTRL			0x0D08
#define CGU_GCR3_RW_DBG_ENABLES			0x0D0C
#define CGU_GCR4_RW_MONITOR_SELECT		0x0D10
#define CGU_GCR5_RW_MONITOR_SB_SELECT		0x0D14
#define CGU_GCR6_RW_DEBUG_OUTPUT_SEL		0x0D18
#define CGU_GCR7_RW_DEBUG_INPUT_SEL		0x0D1C
#define CGU_GCR8_RW_OUT_CLOCK_PHASE_SEL		0x0D20
#define CGU_GCR9_RW_ANA_CLOCK_DIV_SEL		0x0D24
#define CGU_GCR10_RW				0x0D28
#define CGU_GCR11_RW				0x0D2C
#define CGU_GCR12_RW				0x0D30
#define CGU_GCR13_RW				0x0D34
#define CGU_INT_CLR_ENABLE1			0x0FC0
#define CGU_INT_SET_ENABLE1			0x0FC4
#define CGU_INT_STATUS1				0x0FC8
#define CGU_INT_ENABLE1				0x0FCC
#define CGU_INT_CLR_STATUS1			0x0FD0
#define CGU_INT_SET_STATUS1			0x0FD4
#define CGU_INT_CLR_ENABLE0			0x0FD8
#define CGU_INT_SET_ENABLE0			0x0FDC
#define CGU_INT_STATUS0				0x0FE0
#define CGU_INT_ENABLE0				0x0FE4
#define CGU_INT_CLR_STATUS0			0x0FE8
#define CGU_INT_SET_STATUS0			0x0FEC
#define CGU_RBUS				0x0FF4
#define CGU_REXT				0x0FF8
#define CGU_RMID				0x0FFC

#endif /* __SAA7231_CGU_REG_H */
