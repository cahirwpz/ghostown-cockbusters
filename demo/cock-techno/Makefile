TOPDIR := $(realpath ../..)

CLEAN-FILES := data/cock_scene_3.c data/cock-pal.c 

PNG2C.cock-pal := --pixmap gradient,16x22x12

include $(TOPDIR)/build/effect.mk

data/%.c: data/%.svg
	@echo "[ANIM] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(TOPDIR)/effects/anim-polygons/encode.py $< > $@
