# Makefile for linuxtv.org dvb-apps/util/dvbscan

objects  = dvbscan.o  \
	   dvbscan_structutils.o \
           dvbscan_dvb.o \
           dvbscan_atsc.o

binaries = dvbscan

inst_bin = $(binaries)

CPPFLAGS += -I../../lib
LDFLAGS  += -L../../lib/libdvbapi -L../../lib/libdvbcfg -L../../lib/libdvbsec -L../../lib/libucsi
LDLIBS   += -ldvbcfg -lucsi -ldvbsec -ldvbapi -lpthread

.PHONY: all

all: $(binaries)

$(binaries): $(objects)

include ../../Make.rules
