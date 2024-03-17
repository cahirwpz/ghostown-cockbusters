TOPDIR := $(realpath ../..)

include $(TOPDIR)/build/effect.mk

data/ground.c: data/ground.py data/ground-double-full.bmp
	@echo "[GEN] $(DIR)$@"
	$(PYTHON3) $^ > $@