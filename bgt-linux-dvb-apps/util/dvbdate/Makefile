# Makefile for linuxtv.org dvb-apps/util/dvbdate

binaries = dvbdate

inst_bin = $(binaries)

CPPFLAGS += -I../../lib
LDFLAGS  += -L../../lib/libdvbapi -L../../lib/libucsi
LDLIBS   += -ldvbapi -lucsi

.PHONY: all

all: $(binaries)

include ../../Make.rules
