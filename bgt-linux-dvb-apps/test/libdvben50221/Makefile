# Makefile for linuxtv.org dvb-apps/test/libdvben50221

binaries = test-app       \
           test-session   \
           test-transport

CPPFLAGS += -I../../lib
LDLIBS   += ../../lib/libdvbapi/libdvbapi.a ../../lib/libdvben50221/libdvben50221.a ../../lib/libucsi/libucsi.a -lpthread

.PHONY: all

all: $(binaries)

include ../../Make.rules
