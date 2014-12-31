# Makefile for linuxtv.org dvb-apps/util/ttusb_dec_reset

binaries = ttusb_dec_reset

ifneq ($(realpath $(binaries)),)
inst_bin = $(binaries)
endif

LDLIBS += -lusb

.PHONY: all

ifeq ($(ttusb_dec_reset),1)
all: $(binaries)
else
all: clean
endif

include ../../Make.rules
