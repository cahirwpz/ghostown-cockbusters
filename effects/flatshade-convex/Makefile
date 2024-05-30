TOPDIR := $(realpath ../..)

CLEAN-FILES := data/flatshade-pal.c data/ball.c data/pilka.c

LIBS := lib3d

PNG2C.flatshade-pal := --palette flatshade,16
OBJ2C.ball := --scale 110.0 -face-normals
OBJ2C.pilka := --scale 65.0 -face-normals

include $(TOPDIR)/build/effect.mk
