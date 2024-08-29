TOPDIR := $(realpath ../..)

CLEAN-FILES := data/cock_scene_3.c data/cock-pal.c data/ksywka1.c

PNG2C.cock-pal := --pixmap gradient,16x22x12
PNG2C.ksywka1  := --pixmap ks1,128x32x4 --palette ks1_pal,4
include $(TOPDIR)/build/effect.mk

data/%.c: data/%.svg
	@echo "[ANIM] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(TOPDIR)/effects/anim-polygons/encode.py $< > $@
