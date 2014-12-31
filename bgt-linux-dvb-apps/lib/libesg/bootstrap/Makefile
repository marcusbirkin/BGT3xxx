# Makefile for linuxtv.org dvb-apps/lib/libesg/bootstrap

.PHONY: sub-error-bootstrap

sub-error-bootstrap:
	$(error You can't use this makefile directly.)

ifneq ($(lib_name),)

objects += bootstrap/access_descriptor.o \
           bootstrap/provider_discovery_descriptor.o

sub-install += bootstrap

else

includes = access_descriptor.h \
           provider_discovery_descriptor.h

include ../../../Make.rules

lib_name = libesg/bootstrap

endif
