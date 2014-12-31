# Makefile for linuxtv.org dvb-apps

# get DVB API version
VERSION_FILE := "/usr/include/linux/dvb/version.h"

DVB_API_MAJOR := $(word 3, $(shell grep -m1 "DVB_API_VERSION" $(VERSION_FILE)) )
DVB_API_MINOR := $(word 3, $(shell grep -m1 "DVB_API_VERSION_MINOR" $(VERSION_FILE)) )

.PHONY: all clean install update

all clean install:
	$(MAKE) -C lib $@
	$(MAKE) -C test $@
	$(MAKE) -C util $@

update:
	@echo "Pulling changes & updating from master repository"
	hg pull -u
