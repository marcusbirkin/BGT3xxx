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
#include <linux/module.h>

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/i2c.h>

#include "saa7231_priv.h"
#include "saa7231_mod.h"
#include "saa7231_i2c_reg.h"
#include "saa7231_msi.h"
#include "saa7231_i2c.h"


#define SAA7231_I2C_ADAPTERS			4


static const char *adap_name[] = {
	"SAA7231 I2C:0",
	"SAA7231 12C:1",
	"SAA7231 I2C:2",
	"SAA7231 I2C:3"
};

static const u32 i2c_dev[] = {
	I2C0,
	I2C1,
	I2C2,
	I2C3
};

#define SAA7231_I2C_BUS(__x) (i2c_dev[__x])

#define SAA7231_I2C_ADAPTER_NAME(__dev)		\
	(adap_name[__dev])

#define SAA7231_I2C_ADAP(__dev)			\
	(i2c_dev[__dev])

#define SAA7231_I2C_TXBUSY	(TX_FIFO_BLOCK	| TX_FIFO_FULL)
#define SAA7231_I2C_RXBUSY	(RX_FIFO_BLOCK	| RX_FIFO_EMPTY)

static void saa7231_term_xfer(struct saa7231_i2c *i2c, u32 I2C_DEV)
{
	struct saa7231_dev *saa7231 = i2c->saa7231;

	SAA7231_WR(0x00c0, SAA7231_BAR0, I2C_DEV, I2C_CONTROL);
	msleep(10);
	SAA7231_WR(0x0080, SAA7231_BAR0, I2C_DEV, I2C_CONTROL);
	msleep(10);
	SAA7231_WR(0x0000, SAA7231_BAR0, I2C_DEV, I2C_CONTROL);
	msleep(10);
	SAA7231_WR(0x0080, SAA7231_BAR0, I2C_DEV, I2C_CONTROL);
	msleep(10);
	SAA7231_WR(0x00c0, SAA7231_BAR0, I2C_DEV, I2C_CONTROL);

	return;
}

static int saa7231_i2c_hwinit(struct saa7231_i2c *i2c, u32 I2C_DEV)
{
	struct saa7231_dev *saa7231 = i2c->saa7231;
	struct saa7231_config *config = saa7231->config;
	struct i2c_adapter *adapter = &i2c->i2c_adapter;

	int i, err = 0;
	u32 reg;

	reg = SAA7231_RD(SAA7231_BAR0, I2C_DEV, I2C_STATUS);
	if (!(reg & 0xd)) {
		dprintk(SAA7231_ERROR, 1, "Adapter (%02x) %s RESET failed, Exiting !",
			I2C_DEV, adapter->name);
			err = -EIO;
			goto exit;
	}

	SAA7231_WR(0x00cc, SAA7231_BAR0, I2C_DEV, I2C_CONTROL);
	SAA7231_WR(0x1fff, SAA7231_BAR0, I2C_DEV, INT_CLR_ENABLE);
	SAA7231_WR(0x1fff, SAA7231_BAR0, I2C_DEV, INT_CLR_STATUS);
	SAA7231_WR(0x00c1, SAA7231_BAR0, I2C_DEV, I2C_CONTROL);

	for (i = 0; i < 100; i++) {
		reg = SAA7231_RD(SAA7231_BAR0, I2C_DEV, I2C_CONTROL);
		if (reg == 0xc0)
				break;
		msleep(1);

		if (i == 99)
			err = -EIO;
	}

	if (err) {
		dprintk(SAA7231_ERROR, 1, "Adapter (%02x) %s RESET failed",
			I2C_DEV, adapter->name);

			saa7231_term_xfer(i2c, I2C_DEV);
			err = -EIO;
			goto exit;
	}

	switch (i2c->i2c_rate) {
	case SAA7231_I2C_RATE_400:
		switch (config->root_clk) {
		case CLK_ROOT_54MHz:
			SAA7231_WR(0x0021, SAA7231_BAR0, I2C_DEV, I2C_CLOCK_DIVISOR_HIGH);
			SAA7231_WR(0x0047, SAA7231_BAR0, I2C_DEV, I2C_CLOCK_DIVISOR_LOW);
			SAA7231_WR(0x001f, SAA7231_BAR0, I2C_DEV, I2C_SDA_HOLD);
			break;
		case CLK_ROOT_27MHz:
			SAA7231_WR(0x001a, SAA7231_BAR0, I2C_DEV, I2C_CLOCK_DIVISOR_HIGH);
			SAA7231_WR(0x0021, SAA7231_BAR0, I2C_DEV, I2C_CLOCK_DIVISOR_LOW);
			SAA7231_WR(0x0019, SAA7231_BAR0, I2C_DEV, I2C_SDA_HOLD);
			break;
		default:
			break;
		}
		break;
	case SAA7231_I2C_RATE_100:
		switch (config->root_clk) {
		case CLK_ROOT_54MHz:
			SAA7231_WR(0x00d0, SAA7231_BAR0, I2C_DEV, I2C_CLOCK_DIVISOR_HIGH);
			SAA7231_WR(0x010e, SAA7231_BAR0, I2C_DEV, I2C_CLOCK_DIVISOR_LOW);
			SAA7231_WR(0x007c, SAA7231_BAR0, I2C_DEV, I2C_SDA_HOLD);
			break;
		case CLK_ROOT_27MHz:
			SAA7231_WR(0x0068, SAA7231_BAR0, I2C_DEV, I2C_CLOCK_DIVISOR_HIGH);
			SAA7231_WR(0x0087, SAA7231_BAR0, I2C_DEV, I2C_CLOCK_DIVISOR_LOW);
			SAA7231_WR(0x0060, SAA7231_BAR0, I2C_DEV, I2C_SDA_HOLD);
			break;
		default:
			break;
		}
		break;
	default:
		dprintk(SAA7231_ERROR, 1, "Adapter %s Unknown Rate (Rate=0x%02x)",
			adapter->name,
			i2c->i2c_rate);
		break;
	}

	SAA7231_WR(0x1fff, SAA7231_BAR0, I2C_DEV, INT_CLR_ENABLE);
	SAA7231_WR(0x1fff, SAA7231_BAR0, I2C_DEV, INT_CLR_STATUS);
	reg = SAA7231_RD(SAA7231_BAR0, I2C_DEV, I2C_STATUS);
	if (!(reg & 0xd)) {
		dprintk(SAA7231_ERROR, 1,
			"Adapter (%02x) %s has bad state, Exiting !",
			I2C_DEV,
			adapter->name);

		err = -EIO;
		goto exit;
	}
	return 0;

exit:
	return err;
}

static int saa7231_i2c_send(struct saa7231_i2c *i2c, u32 I2C_DEV, u32 data)
{
	struct saa7231_dev *saa7231 = i2c->saa7231;
	int i, err = 0;
	u32 reg;

	reg = SAA7231_RD(SAA7231_BAR0, I2C_DEV, I2C_STATUS);
	if (reg & SAA7231_I2C_TXBUSY) {
		for (i = 0; i < 5; i++) {
			msleep(10);
			reg = SAA7231_RD(SAA7231_BAR0, I2C_DEV, I2C_STATUS);

			if (reg & SAA7231_I2C_TXBUSY) {
				dprintk(SAA7231_ERROR, 1, "ERROR: FIFO full or Blocked, status=0x%02x", reg);

				err = saa7231_i2c_hwinit(i2c, I2C_DEV);
				if (err < 0) {
					dprintk(SAA7231_ERROR, 1, "ERROR: Initialization failed! err=%d", err);
					err = -EIO;
					goto exit;
				}
			} else {
				break;
			}
		}
	}

	SAA7231_WR(data, SAA7231_BAR0, I2C_DEV, TX_FIFO);
	udelay(200);
	for (i = 0; i < 1000; i++) {
		reg = SAA7231_RD(SAA7231_BAR0, I2C_DEV, I2C_STATUS);
		if (reg & TX_FIFO_EMPTY) {
			break;
		}
	}

	if (!(reg & TX_FIFO_EMPTY)) {
		dprintk(SAA7231_ERROR, 1, "ERROR: TXFIFO not empty after timeout, status=0x%02x", reg);
		err = -EIO;
		goto exit;
	}
	return err;

exit:
	dprintk(SAA7231_ERROR, 1, "I2C Send failed (Err=%d)", err);
	return err;
}

static int saa7231_i2c_recv(struct saa7231_i2c *i2c, u32 I2C_DEV, u32 *data)
{
	struct saa7231_dev *saa7231 = i2c->saa7231;
	int i, err = 0;
	u32 reg;

	for (i = 0; i < 100; i++) {
		reg = SAA7231_RD(SAA7231_BAR0, I2C_DEV, I2C_STATUS);
		if (!(reg & SAA7231_I2C_RXBUSY))
			break;
	}
	if (reg & SAA7231_I2C_RXBUSY) {
		dprintk(SAA7231_INFO, 1, "FIFO empty");
		err = -EIO;
		goto exit;
	}

	*data = SAA7231_RD(SAA7231_BAR0, I2C_DEV, RX_FIFO);
	return 0;
exit:
	dprintk(SAA7231_ERROR, 1, "Error Reading data, err=%d", err);
	return err;
}

static int saa7231_i2c_xfer(struct i2c_adapter *adapter, struct i2c_msg *msgs, int num)
{
	struct saa7231_i2c *i2c		= i2c_get_adapdata(adapter);
	struct saa7231_dev *saa7231	= i2c->saa7231;

	u32 DEV = SAA7231_I2C_BUS(i2c->i2c_dev);
	int i = 0, j, err = 0;
	u32 data;
	u32 reg;

	dprintk(SAA7231_DEBUG, 0, "\n");
	dprintk(SAA7231_DEBUG, 1, "Bus(%02x) I2C transfer", DEV);
	mutex_lock(&i2c->i2c_lock);

        reg =  SAA7231_RD(SAA7231_BAR0, DEV, I2C_CONTROL);
        reg |= 0x01;

	SAA7231_WR(reg, SAA7231_BAR0, DEV, I2C_CONTROL);

	for (i = 0; i < num; i++) {

		data = (msgs[i].addr << 1) | I2C_START_BIT;
		if (msgs[i].flags & I2C_M_RD)
			data |= 1;

		dprintk(SAA7231_DEBUG, 0, "\n    len=%d <ST> <%02x> %s", msgs[i].len, data & 0xfe, (data & 0x1) ? "<R> " : "<W> ");
		err = saa7231_i2c_send(i2c, DEV, data);
		if (err < 0) {
			dprintk(SAA7231_ERROR, 1, "Address write failed");
			err = -EIO;
			goto retry;
		}
		for (j = 0; j < msgs[i].len; j++) {
			if (msgs[i].flags & I2C_M_RD)
				data = 0x00;
			else
				data = msgs[i].buf[j];

			if (i == (num - 1) && j == (msgs[i].len - 1))
				data |= I2C_STOP_BIT;

			if (!(msgs[i].flags & I2C_M_RD))
				dprintk(SAA7231_DEBUG, 0, "<%02x> ", (data & 0xff));

			err = saa7231_i2c_send(i2c, DEV, data);
			if (err < 0) {
				dprintk(SAA7231_ERROR, 1, "Data send failed");
				err = -EIO;
				goto retry;
			}
			if (msgs[i].flags & I2C_M_RD) {
				err = saa7231_i2c_recv(i2c, DEV, &data);
				if (err < 0) {
					dprintk(SAA7231_ERROR, 1, "Data receive failed");
					err = -EIO;
					goto retry;
				}
				dprintk(SAA7231_DEBUG, 0, " <R=%02x> %s", data, ((i == (num - 1)) && (j == msgs[i].len - 1)) ? "<SP>\n" : "");
				msgs[i].buf[j] = data;
			}
		}
	}

retry:
	if (err) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Bailing out <%d>", err);
	} else {
		err = num;
	}
	mutex_unlock(&i2c->i2c_lock);
	return err;
}

static u32 saa7231_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm saa7231_algo = {
	.master_xfer	= saa7231_i2c_xfer,
	.functionality	= saa7231_i2c_func,
};

static const char *i2c_rate_desc[] = {
	"Unknown",
	"400kHz",
	"100kHz"
};

int saa7231_i2c_init(struct saa7231_dev *saa7231)
{
	struct pci_dev *pdev		= saa7231->pdev;
	struct saa7231_i2c *i2c		= NULL;
	struct i2c_adapter *adapter	= NULL;

	int i, err = 0;

	mutex_lock(&saa7231->dev_lock);
	i2c = kzalloc((sizeof (struct saa7231_i2c) * SAA7231_I2C_ADAPTERS), GFP_KERNEL);
	if (i2c == NULL) {
		dprintk(SAA7231_ERROR, 1, "ERROR: Could not allocate I2C adapters");
		return -ENOMEM;
	}
	saa7231->i2c = i2c;

	for (i = 0; i < SAA7231_I2C_ADAPTERS; i++) {

		mutex_init(&i2c->i2c_lock);
		i2c->i2c_dev	= i;
		i2c->i2c_rate	= saa7231->config->i2c_rate;
		adapter		= &i2c->i2c_adapter;

		if (adapter != NULL) {

			i2c_set_adapdata(adapter, i2c);
			strcpy(adapter->name, SAA7231_I2C_ADAPTER_NAME(i));

			adapter->owner		= THIS_MODULE;
			adapter->algo		= &saa7231_algo;
			adapter->algo_data 	= NULL;
			adapter->timeout	= 500;
			adapter->retries	= 3;
			adapter->dev.parent	= &pdev->dev;

			dprintk(SAA7231_DEBUG, 1, "Initializing Adapter:%d (0x%02x) %s @ %s",
				i,
				SAA7231_I2C_ADAP(i),
				adapter->name,
				i2c_rate_desc[i2c->i2c_rate]);

			err = i2c_add_adapter(adapter);
			if (err < 0) {
				dprintk(SAA7231_ERROR, 1, "Adapter (%d) %s init failed", i, adapter->name);
				goto exit;
			}

			i2c->saa7231 = saa7231;
			saa7231_i2c_hwinit(i2c, SAA7231_I2C_ADAP(i));
		}
		i2c++;
	}

	dprintk(SAA7231_DEBUG, 1, "SAA%02x I2C Core succesfully initialized", saa7231->pdev->device);
	mutex_unlock(&saa7231->dev_lock);
	return 0;
exit:
	mutex_unlock(&saa7231->dev_lock);
	return err;
}
EXPORT_SYMBOL_GPL(saa7231_i2c_init);

void saa7231_i2c_exit(struct saa7231_dev *saa7231)
{
	struct saa7231_i2c *i2c		= saa7231->i2c;
	struct i2c_adapter *adapter	= NULL;
	int i;

	dprintk(SAA7231_DEBUG, 1, "Removing SAA%02x I2C Core", saa7231->pdev->device);
	mutex_lock(&saa7231->dev_lock);
	for (i = 0; i < SAA7231_I2C_ADAPTERS; i++) {

		adapter = &i2c->i2c_adapter;
		if (adapter) {
			dprintk(SAA7231_DEBUG, 1, "Removing adapter (%d) %s", i, adapter->name);
			i2c_del_adapter(adapter);
		}
		i2c++;
	}
	dprintk(SAA7231_DEBUG, 1, "SAA%02x I2C Core succesfully removed", saa7231->pdev->device);
	i2c = saa7231->i2c;
	kfree(i2c);
	mutex_unlock(&saa7231->dev_lock);
	return;
}
EXPORT_SYMBOL_GPL(saa7231_i2c_exit);
