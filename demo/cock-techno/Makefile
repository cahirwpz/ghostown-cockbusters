TOPDIR := $(realpath ../..)

CLEAN-FILES := data/cock_scene_3.c data/cock-pal.c data/ksywka1.c data/ksywka2.c data/cock-techno.c data/cock-pal10.c data/cock-pal11.c data/cock-pal2.c data/cock-pal3.c data/cock-pal4.c data/cock-pal5.c data/cock-pal6.c data/cock-pal7.c data/cock-pal8.c data/cock-pal9.c

PNG2C.cock-pal := --pixmap gradient1,4x18x12
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

PNG2C.ksywka1  := --sprite ks1,32,8 --palette ks1_pal,4
PNG2C.ksywka2  := --sprite ks2,32,8 --palette ks2_pal,4

include $(TOPDIR)/build/effect.mk

data/%.c: data/%.svg
	@echo "[ANIM] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(TOPDIR)/effects/anim-polygons/encode.py $< > $@
