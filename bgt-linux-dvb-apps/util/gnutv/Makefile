# Makefile for linuxtv.org dvb-apps/util/gnutv

objects  = gnutv_ca.o  \
           gnutv_dvb.o \
           gnutv_data.o

binaries = gnutv

inst_bin = $(binaries)

CPPFLAGS += -I../../lib
LDFLAGS  += -L../../lib/libdvbapi -L../../lib/libdvbcfg -L../../lib/libdvbsec -L../../lib/libdvben50221 -L../../lib/libucsi
LDLIBS   += -ldvbcfg -ldvben50221 -lucsi -ldvbsec -ldvbapi -lpthread

.PHONY: all

all: $(binaries)

$(binaries): $(objects)

include ../../Make.rules
