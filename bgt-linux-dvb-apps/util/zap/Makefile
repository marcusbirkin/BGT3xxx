# Makefile for linuxtv.org dvb-apps/util/zap

objects  = zap_ca.o  \
           zap_dvb.o

binaries = zap

inst_bin = $(binaries)

CPPFLAGS += -I../../lib
LDFLAGS  += -L../../lib/libdvbapi -L../../lib/libdvbsec  -L../../lib/libdvbcfg -L../../lib/libdvben50221 -L../../lib/libucsi
LDLIBS   += -ldvbcfg -ldvben50221 -ldvbsec -ldvbapi -lucsi -lpthread

.PHONY: all

all: $(binaries)

$(binaries): $(objects)

include ../../Make.rules
