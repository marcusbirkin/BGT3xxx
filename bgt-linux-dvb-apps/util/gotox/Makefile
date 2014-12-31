# Makefile for linuxtv.org dvb-apps/util/gotox

binaries = gotox

inst_bin = $(binaries)

CPPFLAGS += -I../../lib
LDFLAGS  += -L../../lib/libdvbapi
LDFLAGS  += -L../../lib/libdvbsec
LDLIBS   += -ldvbapi
LDLIBS   += -ldvbsec

.PHONY: all

all: $(binaries)

include ../../Make.rules
