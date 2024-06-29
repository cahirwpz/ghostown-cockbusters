TOPDIR := $(realpath ../..)

CLEAN-FILES := data/flares32.c data/dna.{c,obj,mtl} \
	data/carrion-metro-{pal,data}.{c,png}

LIBS := lib3d

PNG2C.flares32 := --bitmap bobs,48x512x3,+interleaved --palette bobs,8
OBJ2C.dna := --scale 20 -mesh-name dna_helix
PNG2C.carrion-metro-data := --bitmap carrion,256x256x2,+interleaved
PNG2C.carrion-metro-pal := --pixmap carrion_cols,4x256x12

include $(TOPDIR)/build/effect.mk

%-data.png %-pal.png: %.png
	@echo "[SPLITIMG] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(TOPDIR)/effects/carrion/data/split-img.py 4 $<

%.obj %.mtl: %_gen.py
	@echo "[GEN] $@"
	$(PYTHON3) $*_gen.py $*

.PRECIOUS: data/dna.obj data/dna.mtl
