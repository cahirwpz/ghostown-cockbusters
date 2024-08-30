TOPDIR := $(realpath ../..)

SOURCES := rotator.c mainloop.asm
CLEAN-FILES := data/texture.c

PNG2C.texture-bright := --pixmap texture,128x128x8 --palette bright,16
PNG2C.texture-dark := --palette dark,16

include $(TOPDIR)/build/effect.mk
