# Makefile for linuxtv.org dvb-apps/lib/libesg/transport

.PHONY: sub-error-transport

sub-error-transport:
	$(error You can't use this makefile directly.)

ifneq ($(lib_name),)

objects += transport/session_partition_declaration.o

sub-install += transport

else

includes = session_partition_declaration.h

include ../../../Make.rules

lib_name = libesg/transport

endif
