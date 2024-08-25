TOPDIR := $(realpath ../..)

CLEAN-FILES := data/cock.c data/ring.c data/bkg.c data/mid_beam.c data/side_beam_l.c data/side_beam_r.c \
	data/ufo0.c data/ufo1.c data/ufo2.c data/ufo3.c data/ufo4.c data/ufo5.c

PNG2C.bkg  := --bitmap bkg,320x256x1
PNG2C.ring := --bitmap ring,56x16x3 --palette ring,2
PNG2C.ufo0  := --bitmap ufo0,104x39x4 --palette abduction,2
PNG2C.ufo1  := --bitmap ufo1,104x39x4
PNG2C.ufo2  := --bitmap ufo2,104x39x4
PNG2C.ufo3  := --bitmap ufo3,104x39x4
PNG2C.ufo4  := --bitmap ufo4,104x39x4
PNG2C.ufo5  := --bitmap ufo5,104x39x4

PNG2C.cock        := --sprite cock,24,1
PNG2C.mid_beam    := --sprite mid_beam,200,1
PNG2C.side_beam_l := --sprite side_beam_l,200,1
PNG2C.side_beam_r := --sprite side_beam_r,200,1

include $(TOPDIR)/build/effect.mk
