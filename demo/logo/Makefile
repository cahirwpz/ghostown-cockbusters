TOPDIR := $(realpath ../..)

CLEAN-FILES := data/logo.c

PNG2C.logo := --bitmap logo,320x256x5 --palette logo,32

include $(TOPDIR)/build/effect.mk

data/%-cycling.c: data/%.iff
	@echo "[DRNG2C] $(DIR)$< -> $(DIR)$@"
	$(DRNG2C) $(DRNG2C.$*) -o $@ $<
