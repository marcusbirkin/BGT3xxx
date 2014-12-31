# Makefile for linuxtv.org dvb-apps/test

objects  = hex_dump.o lnb.o

binaries = diseqc          \
           sendburst       \
           set22k          \
           setpid          \
           setvoltage      \
           test_av         \
           test_av_play    \
           test_dvr        \
           test_tapdmx     \
           test_dvr_play   \
           test_pes        \
           test_sec_ne     \
           test_sections   \
           test_stc        \
           test_stillimage \
           test_tt         \
           test_vevent     \
           test_video      \
	   evtest	   \
	   szap2           \
	   lock_s

.PHONY: all

all: $(binaries)
	make -C libdvbcfg $@
	make -C libdvben50221 $@
	make -C libesg $@
	make -C libucsi $@

$(binaries): $(objects)

clean::
	make -C libdvbcfg $@
	make -C libdvben50221 $@
	make -C libesg $@
	make -C libucsi $@

include ../Make.rules
