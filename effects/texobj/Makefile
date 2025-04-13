TOPDIR := $(realpath ../..)

SOURCES := texobj.c mainloop.asm
CLEAN-FILES := data/cube-bg.c data/cube-tex?.c data/cube.c data/texobj.c data/cube-anim.c

LIBS := lib3d

PNG2C.cube-bg := --pixmap background,128x128x8,+displayable,+onlydata --palette texture,16
PNG2C.cube-bg-dark := --palette dark,16
PNG2C.cube-tex0 := --pixmap texture_0,256x128x8,+onlydata
PNG2C.cube-tex1 := --pixmap texture_1,256x128x8,+onlydata
PNG2C.cube-tex2 := --pixmap texture_2,256x128x8,+onlydata
PNG2C.cube-tex3 := --pixmap texture_3,256x128x8,+onlydata
PNG2C.cube-tex4 := --pixmap texture_4,256x128x8,+onlydata
OBJ2C.cube := --scale 48.0 --mesh-name cube --no-edges --textured --texture-scale 2048.0
ANIM2C.cube-anim := --name cube_anim --scale 127

include $(TOPDIR)/build/effect.mk
