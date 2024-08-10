TOPDIR := $(realpath ../..)

#CLEAN-FILES := data/dragon-bg.c data/texture-15.c data/ball.c
CLEAN-FILES := data/dragon-bg.c data/ball.c

#PNG2C.dragon-bg := --bitmap background,320x256x4 --palette background_pal,16
PNG2C.dragon-bg := --pixmap dragon,320x256x4,+displayable --palette dragon_pal,32

include $(TOPDIR)/build/effect.mk
WFLAGS :=   #niech sie kompilator odwali

data/ball.c: data/gen-ball.py
	@echo "[GEN] $@"
	$(PYTHON3) $< uvmap > $@
