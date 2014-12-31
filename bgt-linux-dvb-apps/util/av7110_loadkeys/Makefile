# Makefile for linuxtv.org dvb-apps/util/av7110_loadkeys

binaries = av7110_loadkeys

inst_bin = $(binaries)

removing = input_keynames.h

.PHONY: all

all: $(binaries)

$(binaries): input_keynames.h

input_keynames.h:
	$(SHELL) generate-keynames.sh $@

include ../../Make.rules

install::
	@echo installing av7110 keymaps
	@mkdir -p $(DESTDIR)$(sharedir)/dvb/av7110_loadkeys
	@install -m 644 *.rc5 *.rcmm $(DESTDIR)$(sharedir)/dvb/av7110_loadkeys/
