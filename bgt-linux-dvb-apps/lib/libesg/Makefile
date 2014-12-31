# Makefile for linuxtv.org dvb-apps/lib/libesg

includes = types.h

objects  = types.o

lib_name = libesg

CPPFLAGS += -I../../lib

.PHONY: all

all: library

include bootstrap/Makefile
include encapsulation/Makefile
include representation/Makefile
include transport/Makefile

.PHONY: $(sub-install)

install:: $(sub-install)

$(sub-install):
	$(MAKE) -C $@ install

include ../../Make.rules
