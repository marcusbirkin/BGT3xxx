# Makefile for linuxtv.org dvb-apps/util/dvbtraffic

binaries = dvbtraffic

inst_bin = $(binaries)

CPPFLAGS += -I../../lib
LDFLAGS  += -L../../lib/libdvbapi
LDLIBS   += -ldvbapi

.PHONY: all

all: $(binaries)

include ../../Make.rules
