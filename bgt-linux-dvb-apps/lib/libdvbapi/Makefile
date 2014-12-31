# Makefile for linuxtv.org dvb-apps/lib/libdvbapi

includes = dvbaudio.h \
           dvbca.h    \
           dvbdemux.h \
           dvbfe.h    \
           dvbnet.h   \
           dvbvideo.h

objects  = dvbaudio.o \
           dvbca.o    \
           dvbdemux.o \
           dvbfe.o    \
           dvbnet.o   \
           dvbvideo.o

lib_name = libdvbapi

CPPFLAGS += -I../../lib

.PHONY: all

all: library

include ../../Make.rules
