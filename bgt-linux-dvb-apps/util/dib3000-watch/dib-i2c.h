/*
 * adapted from different kernel headers
 * "this is the current way of doing things."-Greg K-H
 *
 * everything copied from linux kernel 2.6.10 source
 */

#ifndef _DIB_I2C_H
#define _DIB_I2C_H


/* from <linux/i2c.h> */
#define I2C_SLAVE       0x0703
#define I2C_SLAVE_FORCE 0x0706
#define I2C_TENBIT      0x0704
#define I2C_PEC         0x0708
#define I2C_RETRIES     0x0701
#define I2C_TIMEOUT     0x0702

#define I2C_FUNCS       0x0705
#define I2C_RDWR        0x0707
#define I2C_SMBUS       0x0720

struct i2c_msg {
	__u16 addr;
	__u16 flags;
#define I2C_M_RD            0x0001
#define I2C_M_TEN           0x0010
#define I2C_M_NOSTART       0x4000
#define I2C_M_REV_DIR_ADDR  0x2000
#define I2C_M_IGNORE_NAK    0x1000
#define I2C_M_NO_RD_ACK     0x0800
	__u16 len;
	__u8 *buf;
};

/* from <linux/i2c-dev.h> */
struct i2c_rdwr_ioctl_data {
	struct i2c_msg *msgs;
	__u32 nmsgs;
};

#endif
