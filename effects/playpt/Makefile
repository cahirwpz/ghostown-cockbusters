TOPDIR := $(realpath ../..)

MODULE := virgill-amigahub
OBJECTS := data/$(MODULE)-trk.o data/$(MODULE).o
CLEAN-FILES := data/lat2-08.c data/*.smp data/*.trk

LIBS := libpt

PSF2C.lat2-08 := --name latin2 --type console
PTSPLIT.$(MODULE) := --delta

include $(TOPDIR)/build/effect.mk

data/%-trk.o: data/%.trk
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) -I binary -O amiga \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_start=_Module \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_end=_ModuleEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_size=_ModuleSize \
	  $^ $@

data/%-smp.o: data/%.smp
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) -I binary -O amiga \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_start=_Samples \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_end=_SamplesEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_size=_SamplesSize \
	  --rename-section .data=.datachip,alloc,load,data,contents $^ $@
