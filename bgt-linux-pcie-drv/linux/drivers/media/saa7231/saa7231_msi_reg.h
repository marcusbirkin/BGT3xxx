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

#ifndef __SAA7231_MSI_REG_H
#define __SAA7231_MSI_REG_H

#define MSI_DELAY_TIMER				0x0000
#define MSI_INTA_POLARITY			0x0004

#define MSI_CONFIG_BASE				0x0008
#define MSI_CONFIG_OFFST			0x0004

#define MSI_CONFIG(__x)		\
	(MSI_CONFIG_BASE + (MSI_CONFIG_OFFST * __x))

#define MSI_FILTER_ENA_BASE			0x0208
#define MSI_FILTER_ENA_OFFST			0x0004

#define MSI_FILTER_ENA(__x)	\
	(MSI_FILTER_ENA_BASE + (MSI_FILTER_ENA_OFFST * __x))

#define MSI_ADDRESS_COMP_BASE			0x0248
#define MSI_ADDRESS_COMP_OFFST			0x0004

#define MSI_ADDRESS_COMP(__x)	\
	(MSI_ADDRESS_COMP_BASE + (MSI_ADDRESS_COMP_OFFST * __x))

#define MSI_CH_STATUS_BASE			0x0400
#define MSI_CH_STATUS_OFFST			0x0004

#define MSI_CH_STATUS(__x)	\
	(MSI_CH_STATUS_BASE + (MSI_CH_STATUS_OFFST * __x))

#define MSI_HW_DEBUG				0x0500

#define MSI_INT_STATUS_BASE			0x0f80
#define MSI_INT_STATUS_OFFST			0x0004

#define MSI_INT_STATUS(__x)	\
	(MSI_INT_STATUS_BASE + (MSI_INT_STATUS_OFFST * __x))

#define MSI_INT_STATUS_CLR_BASE			0x0f90
#define MSI_INT_STATUS_CLR_OFFST		0x0004

#define MSI_INT_STATUS_CLR(__x)	\
	(MSI_INT_STATUS_CLR_BASE + (MSI_INT_STATUS_CLR_OFFST * __x))

#define MSI_INT_STATUS_SET_BASE			0x0fa0
#define MSI_INT_STATUS_SET_OFFST		0x0004

#define MSI_INT_STATUS_SET(__x)	\
	(MSI_INT_STATUS_SET_BASE + (MSI_INT_STATUS_SET_OFFST * __x))

#define MSI_INT_ENA_BASE			0x0fb0
#define MSI_INT_ENA_OFFST			0x0004

#define MSI_INT_ENA(__x)	\
	(MSI_INT_ENA_BASE + (MSI_INT_ENA_OFFST * __x))

#define MSI_INT_ENA_CLR_BASE			0x0fc0
#define MSI_INT_ENA_CLR_OFFST			0x0004

#define MSI_INT_ENA_CLR(__x)	\
	(MSI_INT_ENA_CLR_BASE + (MSI_INT_ENA_CLR_OFFST * __x))

#define MSI_INT_ENA_SET_BASE			0x0fd0
#define MSI_INT_ENA_SET_OFFST			0x0004

#define MSI_INT_ENA_SET(__x)	\
	(MSI_INT_ENA_SET_BASE + (MSI_INT_ENA_SET_OFFST * __x))

#define MSI_SW_RST				0x0FF0
#define MSI_MODULE_ID				0x0FFC

#endif /* __SAA7231_MSI_REG_H */
