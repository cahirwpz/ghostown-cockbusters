TOPDIR := $(realpath ../..)

CLEAN-FILES := data/rork-128.c data/cube.c

LIBS := lib3d

PNG2C.rork-128 := --pixmap texture,128x128x8,+displayable --palette texture,16
OBJ2C.cube := --scale 128.0 --mesh-name cube --no-edges --textured

include $(TOPDIR)/build/effect.mk
