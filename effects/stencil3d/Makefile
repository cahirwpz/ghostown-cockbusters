TOPDIR := $(realpath ../..)

CLEAN-FILES := data/kurak-head.c data/cock-anim.c \
	             data/background-data.* data/background-pal.* data/pattern-?-?.c

LIBS := lib3d

PNG2C.background-data := --bitmap background,256x256x2
PNG2C.background-pal := --pixmap background_cols,4x256x12,+onlydata
PNG2C.pattern-1-1 := --bitmap pattern_1_1,256x256x2 --palette pattern_1,4
PNG2C.pattern-1-2 := --bitmap pattern_1_2,256x256x2
PNG2C.pattern-1-3 := --bitmap pattern_1_3,256x256x2
PNG2C.pattern-2-1 := --bitmap pattern_2_1,256x256x2 --palette pattern_2,4
PNG2C.pattern-2-2 := --bitmap pattern_2_2,256x256x2
PNG2C.pattern-2-3 := --bitmap pattern_2_3,256x256x2
OBJ2C.kurak-head := --scale 2.1 --mesh-name kurak
ANIM2C.cock-anim := --name cock_anim --scale 127

include $(TOPDIR)/build/effect.mk

%-data.png %-pal.png: %.png
	@echo "[SPLITIMG] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(TOPDIR)/effects/carrion/data/split-img.py 4 $<
