# Makefile for linuxtv.org dvb-apps/lib/libesg/representation

.PHONY: sub-error-representation

sub-error-representation:
	$(error You can't use this makefile directly.)

ifneq ($(lib_name),)

objects += representation/encapsulated_textual_esg_xml_fragment.o \
           representation/init_message.o \
           representation/textual_decoder_init.o

sub-install += representation

else

includes = encapsulated_textual_esg_xml_fragment.h \
           init_message.h \
           textual_decoder_init.h

include ../../../Make.rules

lib_name = libesg/representation

endif
