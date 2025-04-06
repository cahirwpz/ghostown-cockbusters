TOPDIR := $(realpath ../..)

CLEAN-FILES := data/lotus.c data/pattern-?-?.c data/lotus-anim.c \
	             data/flower3d.c data/bg-*.c data/pal-*.c

LIBS := lib3d

PNG2C.bg-3-data := --bitmap bg_3,256x256x2
PNG2C.bg-1-pal := --pixmap bg_1_cols,4x256x12,+onlydata
PNG2C.bg-2-pal := --pixmap bg_2_cols,4x256x12,+onlydata
PNG2C.bg-3-pal := --pixmap bg_3_cols,4x256x12,+onlydata
PNG2C.pal-yellow := --palette pattern_2,4
PNG2C.pal-gray := --palette pal_gray,4
PNG2C.pattern-1-1 := --bitmap pattern_1_1,128x128x2 
PNG2C.pattern-1-2 := --bitmap pattern_1_2,128x128x2 --palette pattern_1,4
PNG2C.pattern-1-3 := --bitmap pattern_1_3,128x128x2
PNG2C.pattern-2-1 := --bitmap pattern_2_1,128x128x2
PNG2C.pattern-2-2 := --bitmap pattern_2_2,128x128x2 
PNG2C.pattern-2-3 := --bitmap pattern_2_3,128x128x2
OBJ2C.lotus := --scale 157.0 --mesh-name flower
ANIM2C.lotus-anim := --name lotus_anim --scale 127

include $(TOPDIR)/build/effect.mk

%-data.png %-pal.png: %.png
	@echo "[SPLITIMG] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(TOPDIR)/effects/carrion/data/split-img.py 4 $<
