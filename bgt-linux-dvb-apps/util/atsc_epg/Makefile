# Makefile for linuxtv.org dvb-apps/util/atsc_epg

binaries = atsc_epg

inst_bin = $(binaries)

CPPFLAGS += -I../../lib -std=c99 -D_POSIX_SOURCE
#LDFLAGS  += -static -L../../lib/libdvbapi -L../../lib/libucsi
LDFLAGS  += -L../../lib/libdvbapi -L../../lib/libucsi
LDLIBS   += -ldvbapi -lucsi

.PHONY: all

all: $(binaries)

include ../../Make.rules
