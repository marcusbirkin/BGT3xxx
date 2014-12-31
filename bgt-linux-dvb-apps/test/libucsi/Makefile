# Makefile for linuxtv.org dvb-apps/test/libucsi

binaries = testucsi

CPPFLAGS += -I../../lib
LDLIBS   += ../../lib/libdvbapi/libdvbapi.a ../../lib/libdvbcfg/libdvbcfg.a \
	    ../../lib/libdvbsec/libdvbsec.a  ../../lib/libucsi/libucsi.a

.PHONY: all

all: $(binaries)

include ../../Make.rules
