TOPDIR := $(realpath ../..)

CLEAN-FILES := data/dualtab.c

include $(TOPDIR)/build/effect.mk

data/dualtab.c: data/gen-dualtab.py
	@echo "[GEN] $(DIR)$@"
	$(PYTHON3) $< > $@
