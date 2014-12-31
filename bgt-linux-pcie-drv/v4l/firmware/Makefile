TARGETS = vicam/firmware.fw dabusb/firmware.fw dabusb/bitstream.bin ttusb-budget/dspbootcode.bin cpia2/stv0672_vp4.bin av7110/bootcode.bin
FW_DIR  = /lib/firmware

####

DIRS = $(sort $(dir $(TARGETS)))

.PHONY = default clean distclean install rminstall prep

default: $(TARGETS)

$(TARGETS):: ihex2fw

clean:
	-rm -f ihex2fw
	-rm -f $(TARGETS)

distclean: clean
	-for i in $(DIRS); do if [ -d $$i ]; then rm -rf $$i; fi; done


install: default
	@echo -n "Installing firmwares at $(FW_DIR): "
	-@for i in $(DIRS); do if [ ! -d $(FW_DIR)/$$i ]; then mkdir -p $(FW_DIR)/$$i; fi; done
	-@for i in $(TARGETS); do echo -n "$$i "; cp $$i $(FW_DIR)/$$i; done
	@echo

rminstall:
	@echo "Removing firmwares at $(FW_DIR) "
	-for i in $(DIRS); do if [ -d $(FW_DIR)/$$i ]; then rm -rf $(FW_DIR)/$$i; fi; done

######

ihex2fw: ../../linux/firmware/ihex2fw.c
	@echo "  CC  $@"
	@gcc -Wall -o $@ $<

prep:
	@for i in $(DIRS); do						\
	if [ ! -d $$i ]; then mkdir -p $$i; fi;				\
	for j in ../../linux/firmware/$$i/*; do				\
		n=`echo $$j | sed s,../../linux/firmware/,,`;		\
		if [ ! -e $$n ]; then					\
			ln -f $$j $$n;					\
		fi;							\
	done; done

%.fw: %.H16
	@echo Generating $@
	@./ihex2fw -w $< $@

%.fw: %.HEX
	@echo Generating $@
	@./ihex2fw $< $@

%.bin: %.bin.ihex
	@echo Generating $@
	@objcopy -Iihex -Obinary $< $@

# TODO: Use the Firmware Kconfig dependencies
#fw-$(CONFIG_USB_VICAM) += vicam/firmware.fw
#fw-$(CONFIG_USB_DABUSB) += dabusb/firmware.fw dabusb/bitstream.bin
#fw-$(CONFIG_DVB_TTUSB_BUDGET) += ttusb-budget/dspbootcode.bin
#fw-$(CONFIG_VIDEO_CPIA2) += cpia2/stv0672_vp4.bin
#fw-$(CONFIG_DVB_AV7110) += av7110/bootcode.bin

