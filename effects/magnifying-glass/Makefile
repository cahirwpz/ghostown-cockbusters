TOPDIR := $(realpath ../..)

CLEAN-FILES := data/logo-gtn.c data/ball.c data/ball-anim.c

PNG2C.logo-gtn := --pixmap logo,320x256x4,+displayable --palette logo_pal,32  --bitmap logo_bp,320x256x4,+displayable
ANIM2C.ball-anim := --name ball_anim

include $(TOPDIR)/build/effect.mk

data/ball.c: data/gen-lens.py
	@echo "[GEN] $@"
	$(PYTHON3) $< uvmap > $@
