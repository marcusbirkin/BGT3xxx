/*
 * $Id: i2c-compat.h,v 1.3 2005/10/16 12:13:58 mchehab Exp $
 *
 * some i2c layer compatibility stuff -- to avoid cluttering up the
 * i2c modules with tons of #ifdefs
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,66)

static inline void *i2c_get_adapdata(struct i2c_adapter *dev)
{
	return dev->data;
}

static inline void i2c_set_adapdata(struct i2c_adapter *dev, void *data)
{
	dev->data = data;
}

static inline void *i2c_get_clientdata(struct i2c_client *dev)
{
	return dev->data;
}

static inline void i2c_set_clientdata(struct i2c_client *dev, void *data)
{
	dev->data = data;
}

#define I2C_DEVNAME(str)   .name = str

static inline char *i2c_clientname(struct i2c_client *c)
{
	return c->name;
}

static inline void i2c_clients_command(struct i2c_adapter *adap,
				       unsigned int cmd, void *arg)
{
	int i;

	for (i = 0; i < I2C_CLIENT_MAX; i++) {
		if (NULL == adap->clients[i])
			continue;
		if (NULL == adap->clients[i]->driver->command)
			continue;
		adap->clients[i]->driver->command(adap->clients[i],cmd,arg);
	}
}

#endif

/*
 * Local variables:
 * c-basic-offset: 8
 * End:
 */
