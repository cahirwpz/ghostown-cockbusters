TOPDIR := $(realpath ../..)

CLEAN-FILES := data/carrion-conan-{pal,data}.{c,png}

PNG2C.carrion-conan-data := --bitmap conan,320x256x2
PNG2C.carrion-conan-pal := --pixmap conan_cols,4x256x12

include $(TOPDIR)/build/effect.mk

%-data.png %-pal.png: %.png
	@echo "[SPLITIMG] $(DIR)$< -> $(DIR)$@"
	$(PYTHON3) data/split-img.py 4 $<
