# Makefile for linuxtv.org dvb-apps/util/szap

# get DVB API version
VERSION_FILE := "/usr/include/linux/dvb/version.h"

DVB_API_MAJOR := $(word 3, $(shell grep -m1 "DVB_API_VERSION" $(VERSION_FILE)) )
DVB_API_MINOR := $(word 3, $(shell grep -m1 "DVB_API_VERSION_MINOR" $(VERSION_FILE)) )

objects  = lnb.o util.o

binaries = azap  \
           czap  \
           szap  \
           tzap

inst_bin = $(binaries)

.PHONY: all

all: $(binaries)
$(info DVB API is $(DVB_API_MAJOR).$(DVB_API_MINOR))

$(binaries): $(objects)

include ../../Make.rules
