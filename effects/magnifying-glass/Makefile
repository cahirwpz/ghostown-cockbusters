TOPDIR := $(realpath ../..)

CLEAN-FILES := data/dragon-bg.c data/ball.c

PNG2C.dragon-bg := --pixmap dragon,320x256x4 --palette dragon_pal,32
include $(TOPDIR)/build/effect.mk

data/ball.c: data/gen-ball.py
	@echo "[GEN] $@"
	$(PYTHON3) $< uvmap > $@
