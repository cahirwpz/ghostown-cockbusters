TOPDIR := $(realpath ..)

# the order here must be the same as in main.c and demo.json !!!
SUBDIRS := loader playpt dna3d stencil3d anim-polygons texobj \
           flower3d logo rotator textscroll abduction

SOURCES := main.c
LOADABLES := xenocoq.exe $(foreach dir, $(SUBDIRS), $(dir).exe)
MAIN := #

DEMO := 1
export DEMO

CPPFLAGS += -I.
LDFLAGS_EXTRA :=
LDSCRIPT := demo.lds
LIBS := libpt lib3d

include $(TOPDIR)/build/effect.mk

$(LOADABLES): demo.exe
	@echo "[SPLITEXE] $< -> $(LOADABLES)"
	$(SPLITEXE) $< $(LOADABLES)

demo.lds: demo.json
	@echo "[LDS] $< -> $@"
	$(PYTHON3) gen-lds.py $< $@

CLEAN-FILES += demo.lds data/demo.c
