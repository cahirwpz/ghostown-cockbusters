TOPDIR := $(realpath ..)

# the order here must be the same as in demo.json !!!
SUBDIRS := empty loader playpt dna3d stencil3d cock-folk texobj \
           flower3d logo rotator textscroll abduction logo_gtn \
					 cock-techno

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

$(LOADABLES): demo.exe $(foreach dir, $(SUBDIRS), $(dir)/$(dir).exe)
	@echo "[SPLITEXE] $< -> $(LOADABLES)"
	$(SPLITEXE) $< $(LOADABLES)
	$(PACKEXE) -algo zx0 $(firstword $(LOADABLES))
	for exe in $(foreach dir, $(SUBDIRS), $(dir).exe); do \
		$(PACKEXE) -algo lzsa1 $$exe; \
	done

demo.lds: demo.json
	@echo "[LDS] $< -> $@"
	$(PYTHON3) gen-lds.py $< $@

CLEAN-FILES += demo.lds data/demo.c
