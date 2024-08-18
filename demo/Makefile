TOPDIR := $(realpath ..)

SUBDIRS := playpt anim-polygons dna3d stencil3d texobj

DELTA := 1

DEMONAME = Demo

LDSCRIPT := demo.lds

CLEAN-FILES := data/loader.c

SOURCES := main.c load.c empty.c
LOADABLES := spooky-town.exe playpt.exe anim-polygons.exe dna3d.exe stencil3d.exe texobj.exe
MAIN := #

DEMO := 1
export DEMO

CPPFLAGS += -I.
LDFLAGS_EXTRA :=
LIBS := libpt lib3d

PNG2C.loader := --bitmap loader,64x64x1 --palette loader,2

include $(TOPDIR)/build/effect.mk

$(LOADABLES): demo.exe
	@echo "[SPLITEXE] $< -> $(LOADABLES)"
	$(SPLITEXE) $< $(LOADABLES)

demo.lds: demo.json
	@echo "[LDS] $< -> $@"
	$(PYTHON3) gen-lds.py $< $@

CLEAN-FILES += demo.lds

REPOHEAD := $(shell git rev-parse --short HEAD)

%.exe.packed: %.exe
	@echo "[PACK] $(DIR)$< -> $(DIR)$@"
	Shrinkler -o -f 0xdff180 \
	  -t "$(DEMONAME) by Ghostown (build: $(REPOHEAD))" $< $@
