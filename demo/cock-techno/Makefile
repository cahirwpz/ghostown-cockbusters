TOPDIR := $(realpath ../..)

CLEAN-FILES := data/cahir.c data/cock-pal10.c data/cock-pal11.c data/cock-pal2.c data/cock-pal3.c data/cock-pal4.c data/cock-pal5.c data/cock-pal6.c data/cock-pal7.c data/cock-pal8.c data/cock-pal9.c data/cock-pal.c data/cock_scene_3.c data/cock-techno.c data/codi.c data/jazzcat.c data/polprog.c data/slayer.c data/slizgi.c data/spook.c data/yumi.c data/zuko.c


PNG2C.cock-pal   := --pixmap gradient1,4x18x12
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

PNG2C.cahir   := --sprite cahir,32,8    --palette cahir,4
PNG2C.codi    := --sprite codi,32,8     --palette codi,4
PNG2C.jazzcat := --sprite jazzcat,32,8  --palette jazzcat,4
PNG2C.polprog := --sprite polprog,32,8  --palette polprog,4
PNG2C.slayer  := --sprite slayer,32,8   --palette slayer,4
PNG2C.slizgi  := --sprite slizgi,32,8   --palette slizgi,4
PNG2C.spook   := --sprite spook,32,8    --palette spook,4
PNG2C.yumi    := --sprite yumi,32,8     --palette yumi,4
PNG2C.zuko    := --sprite zuko,32,8     --palette zuko,4



include $(TOPDIR)/build/effect.mk

data/%.c: data/%.svg
	@echo "[ANIM] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(TOPDIR)/effects/anim-polygons/encode.py $< > $@
