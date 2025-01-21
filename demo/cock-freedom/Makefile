TOPDIR := $(realpath ../..)

CLEAN-FILES := data/cock_scene_3.c data/cock-pal*.c 

PNG2C.cock-pal   := --pixmap gradient,4x18x12
PNG2C.cock-pal2  := --pixmap gradient2,4x18x12
PNG2C.cock-pal3  := --pixmap gradient3,4x18x12
PNG2C.cock-pal4  := --pixmap gradient4,4x18x12
PNG2C.cock-pal5  := --pixmap gradient5,4x18x12
PNG2C.cock-pal6  := --pixmap gradient6,4x18x12
PNG2C.cock-pal7  := --pixmap gradient7,4x18x12
PNG2C.cock-pal8  := --pixmap gradient8,4x18x12
PNG2C.cock-pal9  := --pixmap gradient9,4x18x12
PNG2C.cock-pal10 := --pixmap gradient10,4x18x12
PNG2C.cock-pal11 := --pixmap gradient11,4x18x12

include $(TOPDIR)/build/effect.mk

data/%.c: data/%.svg
	@echo "[ANIM] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(TOPDIR)/effects/anim-polygons/encode.py $< > $@
