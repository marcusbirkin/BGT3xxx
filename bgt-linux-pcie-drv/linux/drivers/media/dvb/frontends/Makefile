#
# Makefile for the kernel DVB frontend device drivers.
#

EXTRA_CFLAGS += -Idrivers/media/dvb/dvb-core/
EXTRA_CFLAGS += -Idrivers/media/common/tuners/

cxd2820r-objs = cxd2820r_core.o cxd2820r_c.o cxd2820r_t.o cxd2820r_t2.o

obj-$(CONFIG_DVB_PLL) += dvb-pll.o
obj-$(CONFIG_DVB_TDA18272) += tda18272.o
obj-$(CONFIG_DVB_TDA10048) += tda10048.o
obj-$(CONFIG_DVB_S5H1411) += s5h1411.o
obj-$(CONFIG_DVB_DUMMY_FE) += dvb_dummy_fe.o
obj-$(CONFIG_DVB_STV090x) += stv090x.o
obj-$(CONFIG_DVB_STV6110x) += stv6110x.o
obj-$(CONFIG_DVB_LNBP21) += lnbp21.o
obj-$(CONFIG_DVB_CXD2820R) += cxd2820r.o
# obj-$(CONFIG_DVB_CXD2834) += cxd2834.o
obj-$(CONFIG_DVB_CXD2850) += cxd2850.o
obj-$(CONFIG_DVB_A8290) += a8290.o
obj-$(CONFIG_DVB_CXD2817) += cxd2817.o
obj-$(CONFIG_DVB_CXD2861) += cxd2861.o
