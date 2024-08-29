TOPDIR := $(realpath ../..)

CLEAN-FILES := data/bobs.c data/bobs2.c data/bobs_gradient.c data/bobs_gradient2.c data/dna.* \
	data/necrocoq-??.c data/necrocoq2-??.c

LIBS := lib3d

PNG2C.bobs  := --bitmap bobs,48x512x3,+interleaved --palette bobs,8
PNG2C.bobs2 := --bitmap bobs2,48x512x3,+interleaved
PNG2C.bobs_gradient  := --pixmap bobs_cols,3x256x12
PNG2C.bobs_gradient2 := --pixmap bobs_cols2,3x256x12
OBJ2C.dna := --scale 35 -mesh-name dna_helix
PNG2C.necrocoq-00-data := --bitmap necrocoq,256x256x2,+interleaved
PNG2C.necrocoq-00-pal := --pixmap necrocoq_00_cols,4x256x12,+onlydata
PNG2C.necrocoq-01 := --pixmap necrocoq_01_cols,4x256x12,+onlydata
PNG2C.necrocoq-02 := --pixmap necrocoq_02_cols,4x256x12,+onlydata
PNG2C.necrocoq-03 := --pixmap necrocoq_03_cols,4x256x12,+onlydata
PNG2C.necrocoq-04 := --pixmap necrocoq_04_cols,4x256x12,+onlydata
PNG2C.necrocoq-05 := --pixmap necrocoq_05_cols,4x256x12,+onlydata
PNG2C.necrocoq-06 := --pixmap necrocoq_06_cols,4x256x12,+onlydata
PNG2C.necrocoq-07 := --pixmap necrocoq_07_cols,4x256x12,+onlydata
PNG2C.necrocoq-08 := --pixmap necrocoq_08_cols,4x256x12,+onlydata
PNG2C.necrocoq-09 := --pixmap necrocoq_09_cols,4x256x12,+onlydata
PNG2C.necrocoq-10 := --pixmap necrocoq_10_cols,4x256x12,+onlydata

PNG2C.necrocoq2-00 := --pixmap necrocoq2_00_cols,4x256x12,+onlydata
PNG2C.necrocoq2-01 := --pixmap necrocoq2_01_cols,4x256x12,+onlydata
PNG2C.necrocoq2-02 := --pixmap necrocoq2_02_cols,4x256x12,+onlydata
PNG2C.necrocoq2-03 := --pixmap necrocoq2_03_cols,4x256x12,+onlydata
PNG2C.necrocoq2-04 := --pixmap necrocoq2_04_cols,4x256x12,+onlydata
PNG2C.necrocoq2-05 := --pixmap necrocoq2_05_cols,4x256x12,+onlydata
PNG2C.necrocoq2-06 := --pixmap necrocoq2_06_cols,4x256x12,+onlydata
PNG2C.necrocoq2-07 := --pixmap necrocoq2_07_cols,4x256x12,+onlydata
PNG2C.necrocoq2-08 := --pixmap necrocoq2_08_cols,4x256x12,+onlydata
PNG2C.necrocoq2-09 := --pixmap necrocoq2_09_cols,4x256x12,+onlydata
PNG2C.necrocoq2-10 := --pixmap necrocoq2_10_cols,4x256x12,+onlydata

include $(TOPDIR)/build/effect.mk

%-data.png %-pal.png: %.png
	@echo "[SPLITIMG] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) $(TOPDIR)/effects/carrion/data/split-img.py 4 $<

%.obj %.mtl: %_gen.py
	@echo "[GEN] $@"
	$(PYTHON3) $*_gen.py --circular $*

.PRECIOUS: data/dna.obj data/dna.mtl
