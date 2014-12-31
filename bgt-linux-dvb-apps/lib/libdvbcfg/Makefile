# Makefile for linuxtv.org dvb-apps/lib/libdvbcfg

includes = dvbcfg_zapchannel.h \
	   dvbcfg_scanfile.h

objects  = dvbcfg_zapchannel.o \
	   dvbcfg_scanfile.o \
	   dvbcfg_common.o

lib_name = libdvbcfg

CPPFLAGS += -I../../lib

.PHONY: all

all: library

include ../../Make.rules
