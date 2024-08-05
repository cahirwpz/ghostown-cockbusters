TOPDIR := $(realpath ../..)

SOURCES := texobj.c mainloop.asm
CLEAN-FILES := data/rork-128.c data/cube.c

LIBS := lib3d

PNG2C.rork-128 := --pixmap texture,128x128x8,+displayable,+onlydata --palette texture,16
OBJ2C.cube := --scale 64.0 --mesh-name cube --no-edges --textured --texture-scale 1024.0

include $(TOPDIR)/build/effect.mk
