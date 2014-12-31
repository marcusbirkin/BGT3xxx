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

#ifndef __SAA7231_I2C_REG_H
#define __SAA7231_I2C_REG_H

#define RX_FIFO					0x00000000
#define TX_FIFO					0x00000000

#define I2C_STOP_BIT				(0x01 <<  9)
#define I2C_START_BIT				(0x01 <<  8)
#define DATA_BYTE_W				(0xff <<  0)


#define TXS_FIFO				0x00000004
#define DLLVAL					(0xff <<  0)

#define I2C_STATUS				0x00000008
#define TX_FIFO_BLOCK				(1 << 11)
#define RX_FIFO_BLOCK				(1 << 10)
#define TXS_FIFO_FULL				(1 <<  9)
#define TXS_FIFO_EMPTY				(1 <<  8)
#define TX_FIFO_FULL				(1 <<  7)
#define TX_FIFO_EMPTY				(1 <<  6)
#define RX_FIFO_FULL				(1 <<  5)
#define RX_FIFO_EMPTY				(1 <<  4)
#define SDA					(1 <<  3)
#define SCL					(1 <<  2)
#define I2C_BUS_ACTIVE				(1 <<  1)
#define MST_SLV_MODE				(1 <<  0)


#define TXS_FIFO				0x00000004
#define I2C_STATUS				0x00000008
#define I2C_CONTROL				0x0000000c
#define I2C_CLOCK_DIVISOR_HIGH			0x00000010
#define I2C_CLOCK_DIVISOR_LOW			0x00000014
#define I2C_ADDRESS				0x00000018
#define RX_LEVEL				0x0000001c
#define TX_LEVEL				0x00000020
#define TXS_LEVEL				0x00000024
#define I2C_SDA_HOLD				0x00000028
#define MODULE_CONF				0x00000fd4
#define INT_CLR_ENABLE				0x00000fd8
#define INT_SET_ENABLE				0x00000fdc
#define INT_STATUS				0x00000fe0
#define INT_ENABLE				0x00000fe4
#define INT_CLR_STATUS				0x00000fe8
#define INT_SET_STATUS				0x00000fec
#define MODULE_ID				0x00000ffc

#endif /* __SAA7231_I2C_REG_H */
