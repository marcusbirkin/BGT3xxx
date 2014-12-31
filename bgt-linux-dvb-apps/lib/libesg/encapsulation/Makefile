# Makefile for linuxtv.org dvb-apps/lib/libesg/encapsulation

.PHONY: sub-error-encapsulation

sub-error-encapsulation:
	$(error You can't use this makefile directly.)

ifneq ($(lib_name),)

objects += encapsulation/container.o \
           encapsulation/fragment_management_information.o \
           encapsulation/data_repository.o \
           encapsulation/string_repository.o

sub-install += encapsulation

else

includes = container.h \
           fragment_management_information.h \
           data_repository.h \
           string_repository.h

include ../../../Make.rules

lib_name = libesg/encapsulation

endif
