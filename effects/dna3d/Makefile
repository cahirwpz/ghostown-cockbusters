TOPDIR := $(realpath ../..)

CLEAN-FILES := data/flares32.c data/dna2.c \
	data/carrion-metro-{pal,data}.{c,png}

LIBS := lib3d

PNG2C.flares32 := --bitmap bobs,48x512x3 --palette bobs,8
OBJ2C.dna2 := --scale 13.5 -mesh-name dna_helix
PNG2C.carrion-metro-data := --bitmap carrion,256x256x2
PNG2C.carrion-metro-pal := --pixmap carrion_cols,4x256x12

include $(TOPDIR)/build/effect.mk

%-data.png %-pal.png: %.png
	@echo "[SPLITIMG] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(TOPDIR)/effects/carrion/data/split-img.py 4 $<
