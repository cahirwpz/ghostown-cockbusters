TOPDIR := $(realpath ..)

DELTA := 1

DEMONAME = Demo
MODULE := JazzCat-ElectricLifeforms

OBJECTS := data/$(MODULE).trk.o

ifeq ($(DELTA), 1)
OBJECTS += data/$(MODULE)-Delta.smp.o
else
OBJECTS += data/$(MODULE).smp.o
endif

LDSCRIPT := demo.lds

LIBS := libpt

CLEAN-FILES := data/$(MODULE)*.o data/$(MODULE).{smp,trk}* data/loader.c
CLEAN-FILES += data/skulls_map.c data/light.c data/skulls_128col.c
CLEAN-FILES += data/texture-rgb.c data/uvmap-rgb.c

SOURCES := main.c load.c empty.c uvmap-rgb.c bumpmap-rgb.c
LOADABLES := spooky-town.exe uvmap-rgb.exe bumpmap-rgb.exe
MAIN := #

CPPFLAGS += -DDEMO=1 -DDELTA=$(DELTA) -I.

PNG2C.loader := --bitmap loader,64x64x1 --palette loader,2
PNG2C.skulls_128col := --pixmap skulls,160x128x8 --palette skulls,128
PNG2C.texture-rgb := --pixmap image,128x128x12

include $(TOPDIR)/build/effect.mk

bumpmap-rgb.o : CFLAGS := $(filter-out -msmall-code, $(CFLAGS))
uvmap-rgb.o : CFLAGS := $(filter-out -msmall-code, $(CFLAGS))

$(LOADABLES): demo.exe
	@echo "[SPLITEXE] $^ -> $(LOADABLES)"
	$(SPLITEXE) $^ $(LOADABLES)

data/skulls_map.c: data/bumpmap.py data/skulls_map.png
	@echo "[GEN] $@"
	$(PYTHON3) $^ bumpmap > $@

data/light.c: data/light.py
	@echo "[GEN] $@"
	$(PYTHON3) $^ light > $@

data/uvmap-rgb.c: data/gen-uvmap.py
	@echo "[GEN] $@"
	$(PYTHON3) $< uvmap > $@

data/%.trk data/%.smp: data/%.mod
	$(PYTHON3) $(TOPDIR)/effects/playpt/data/ptsplit.py $^
	mv data/$*.mod.trk data/$*.trk
	mv data/$*.mod.smp data/$*.smp

demo.lds: demo.json
	@echo "[LDS] $< -> $@"
	$(PYTHON3) gen-lds.py $< $@

CLEAN-FILES += demo.lds

OBJCOPY-CHIP := --rename-section .data=.datachip,alloc,load,data,contents

data/%.trk.o: data/%.trk
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) -I binary -O amiga \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_start=_Module \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_end=_ModuleEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_size=_ModuleSize \
	  $^ $@

data/%-Delta.smp: data/%.smp
	@echo "[DELTA] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) delta.py $< $@

data/%.smp.o: data/%.smp
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) $(OBJCOPY-CHIP) -I binary -O amiga \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_start=_Samples \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_end=_SamplesEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_size=_SamplesSize \
	  $^ $@

data/%.o: data/%.txt
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) -I binary -O amiga \
	  --redefine-sym _binary_data_$(subst -,_,$*)_txt_start=_Text \
	  --redefine-sym _binary_data_$(subst -,_,$*)_txt_end=_TextEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_txt_size=_TextSize \
	  $^ $@

REPOHEAD := $(shell git rev-parse --short HEAD)

%.exe.packed: %.exe
	@echo "[PACK] $(DIR)$< -> $(DIR)$@"
	Shrinkler -o -f 0xdff180 \
	  -t "$(DEMONAME) by Ghostown (build: $(REPOHEAD))" $< $@
