#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <sprite.h>
#include <system/memory.h>
#include <system/interrupt.h>
#include <common.h>
#include <2d.h>
#include <color.h>

#include "data/abduction.c"

#include "data/naked.c"
#include "data/talk.c"
#include "data/sing.c"
#include "data/bkg.c"
#include "data/ring.c"
#include "data/cock.c"
#include "data/mid_beam.c"
#include "data/side_beam_l.c"
#include "data/side_beam_r.c"
#include "data/ufo0.c"
#include "data/ufo1.c"
#include "data/ufo2.c"
#include "data/ufo3.c"
#include "data/ufo4.c"
#include "data/ufo5.c"


#define WIDTH 320
#define HEIGHT 256
#define DEPTH 4

#define RING_W 56
#define RING_H 16

#define UFO_W 104
#define UFO_H 39


typedef enum phase {
  FADEIN,
  ABDUCT,
  RETRACT_BEAM,
  ESCAPE,
  FADE_OUT,
  END
} phaseE;


static const __data  BitmapT *ufo[6] = {&ufo0, &ufo1, &ufo2, &ufo3, &ufo4, &ufo5};
static __code BitmapT *screen[2];
static __code CopInsPairT *bplptr;
static __code CopInsPairT *sprptr;
static __code CopInsT *ring_pal;
static __code CopListT *cp;
static __code short active = 0;

static __code phaseE phase = FADEIN;

static __code short ufo_idx = 0;
static __code short ufo_pos = 25;
static __code short cock_pos = 255-24;
static __code short cock_speed = 2;
static __code short beam_pos[2] = {X(137), X(167)};

static __code short active_pal = 0;
static short beam_pal[4][7] = {
  {0xCEF, 0xCEF, 0xCEF, 0xF0F, 0x2AD, 0x079, 0x046},
  {0xDFF, 0xDFF, 0xDFF, 0xF0F, 0x3BE, 0x18A, 0x157},
  {0xFFF, 0xFFF, 0xFFF, 0xF0F, 0x4CF, 0x29B, 0x268},
  {0xDFF, 0xDFF, 0xDFF, 0xF0F, 0x3BE, 0x18A, 0x157},
};

#define pal_count 32
static const u_short pal[pal_count] = {
  [0]  = 0x034,
  [1]  = 0x000,
  [2]  = 0xFFF,
  [3]  = 0x9EF,
  [4]  = 0x6DF,
  [5]  = 0x3BE,
  [6]  = 0x29B,
  [7]  = 0x289,
  [8]  = 0x157,
  [9]  = 0x035,
  [10] = 0x034,

  // Unused
  [11] = 0xDDF,
  [12] = 0xDDF,
  [13] = 0xDDF,
  [14] = 0xDDF,
  [15] = 0xDDF,

  [16] = 0xF0F,
  [17] = 0xF0F,
  [18] = 0xF0F,
  [19] = 0xF0F,

  // Coq
  [20] = 0x000,
  [21] = 0x035,
  [22] = 0x2AD,
  [23] = 0x00F,
  // Beam
  [24] = 0x000,
  [25] = 0xCEF,
  [26] = 0xCEF,
  [27] = 0xCEF,

  [28] = 0x000,
  [29] = 0x2AD,
  [30] = 0x079,
  [31] = 0x046,
};


static void BlitterFadeIn(void** planes, short bpi, short idx, short y, short x, u_short quote[], short w) {
  static short tab1[16] = {
    0x1000, 0x1010, 0x1011, 0x1111,
    0x1131, 0x3131, 0x3171, 0x3371,
    0x3771, 0x3773, 0x7773, 0x7E73,
    0x7F77, 0xFF77, 0xFF7F, 0xFFFF,
  };
  static short tab2[16] = {
    0x0010, 0x0014, 0x1041, 0x1051,
    0x1451, 0x5451, 0xD451, 0xD455,
    0xDC55, 0xDC5D, 0xDCDD, 0xDCFD,
    0xDDFD, 0xFDFD, 0xFDFF, 0xFFFF,
  };

  custom->bltamod = w;
  custom->bltbmod = 0;
  custom->bltcmod = 10;
  custom->bltdmod = (40 - w) + 40;
  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltcon1 = 0;

  custom->bltbdat = tab2[idx];
  custom->bltapt = &quote[w/2];
  custom->bltdpt = planes[bpi] + 40 + (y * 40) + x;

  custom->bltcon0 = (SRCA | DEST) | A_AND_B;
  custom->bltsize = (16 << 6) | (w / 2);
  WaitBlitter();

  custom->bltamod = w;
  custom->bltbmod = 0;
  custom->bltcmod = 10;
  custom->bltdmod = (40 - w) + 40;
  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltcon1 = 0;

  custom->bltbdat = tab1[idx];
  custom->bltapt = quote;
  custom->bltdpt = planes[bpi] + (y * 40) + x;

  custom->bltcon0 = (SRCA | DEST) | A_AND_B;
  custom->bltsize = (16 << 6) | (w / 2);
  WaitBlitter();
}

static void BlitterFadeOut(void** planes, short bpi, short idx, short y, short x, u_short quote[], short w) {
  static short tab1[16] = {
    0x1000, 0x1010, 0x1011, 0x1111,
    0x1131, 0x3131, 0x3171, 0x3371,
    0x3771, 0x3773, 0x7773, 0x7E73,
    0x7F77, 0xFF77, 0xFF7F, 0xFFFF,
  };
  static short tab2[16] = {
    0x0010, 0x0014, 0x1041, 0x1051,
    0x1451, 0x5451, 0xD451, 0xD455,
    0xDC55, 0xDC5D, 0xDCDD, 0xDCFD,
    0xDDFD, 0xFDFD, 0xFDFF, 0xFFFF,
  };

  custom->bltamod = w;
  custom->bltbmod = 0;
  custom->bltcmod = 10;
  custom->bltdmod = (40 - w) + 40;
  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltcon1 = 0;

  custom->bltbdat = ~tab2[idx];
  custom->bltapt = &quote[w/2];
  custom->bltdpt = planes[bpi] + 40 + (y * 40) + x;

  custom->bltcon0 = (SRCA | DEST) | A_AND_B;
  custom->bltsize = (16 << 6) | (w / 2);
  WaitBlitter();

  custom->bltamod = w;
  custom->bltbmod = 0;
  custom->bltcmod = 10;
  custom->bltdmod = (40 - w) + 40;
  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltcon1 = 0;

  custom->bltbdat = ~tab1[idx];
  custom->bltapt = quote;
  custom->bltdpt = planes[bpi] + (y * 40) + x;

  custom->bltcon0 = (SRCA | DEST) | A_AND_B;
  custom->bltsize = (16 << 6) | (w / 2);
  WaitBlitter();
}

static void DrawBackground(BitmapT *dst) {
  BitmapCopyArea(dst, 0, 0, &bkg, &((Area2D){0, 0, WIDTH, HEIGHT}));
}

static void DrawUfo(BitmapT *dst) {
  Area2D ufo_area = {0, 0, UFO_W, UFO_H};
  short h = ufo_pos;

  if (ufo_pos <= -39) {
    return;
  }

  if (ufo_pos <= 0) {
    ufo_area.y = -ufo_pos;
    ufo_area.h = 39 + ufo_pos;
    h = 0;
  }

  BitmapCopyArea(dst, 109, h, ufo[ufo_idx], &ufo_area);

  ufo_idx = (ufo_idx + 1) % 6;
}

static void DrawRing(BitmapT *dst, short height) {
  Area2D ring_area = {0, 0, RING_W, RING_H};

  if (height < 64) {
    ring_area.y = 64 - height;
    ring_area.h = RING_H - (64 - height);
    height = 64;
  }

  if (height + RING_H >= HEIGHT - 16) {
    ring_area.h = HEIGHT - height - 16;
  }

  if (ring_area.y < 0) return;
  if (ring_area.h <= 0) return;
  if (ring_area.h > RING_H) return;

  BitmapCopyArea(dst, 133, height, &ring, &ring_area);
}

static void SwitchBeamPal(void) {
  short i = 0;

  for (i = 0; i < 7; ++i) {
    SetColor(25 + i, beam_pal[active_pal][i]);
  }

  active_pal = (active_pal + 1) % 4;
}


static void Abduct(void) {
  static short mod = 1;
  short i = 0;

  for (i = 64+16; i <= 256 - 16; i += RING_H*2) {
    DrawRing(screen[active], i - mod);
  }

  if (frameCount % 1 == 0) {
    if (mod < RING_H*2 - 1) {
      ++mod;
    }
    else {
      mod = 0;
    }
  }

  if (frameCount % cock_speed == 0) {
    if (cock_pos > 32) {
      --cock_pos;
      SpriteUpdatePos(&cock,  X(152), Y(cock_pos));
    } else {
      phase = RETRACT_BEAM;
    }
  }

  if (frameCount % 8 == 0) {
    SwitchBeamPal();
  }
}

static void RetractBeam(void) {
  static unsigned short beam_gradient[15] = {
    0xcef,0xcdf,0xbce,0xacd,0x9bc,0x8ab,0x79a,0x689,
    0x578,0x478,0x367,0x356,0x245,0x144,0x034
  };
  static short idx = 0;
  static short h = 224;
  static short s = 0;
  short i = 0;
  CopInsT *ins = ring_pal + 2;
  Area2D ring_area = {133, h, RING_W, RING_H};

  if (frameCount % 2 == 0) {
    for (i = 2; i < 8; ++i) {
      CopInsSet16(ins++, ColorTransition(ring_colors[i], 0x034, s));
      // CopInsSet16(ins++, beam_gradient[s]);
    }
    ++s;

    active_pal = 0;
    beam_pal[0][0] = beam_gradient[idx];
    beam_pal[0][1] = beam_gradient[idx];
    beam_pal[0][2] = beam_gradient[idx];
    SwitchBeamPal();
    ++idx;

    SpriteUpdatePos(&side_beam_l, ++beam_pos[0], Y(56));
    SpriteUpdatePos(&side_beam_r, --beam_pos[1], Y(56));

    if (h >= 64) {
      BitmapClearArea(screen[0], &ring_area);
      BitmapClearArea(screen[1], &ring_area);
      h -= 16;
    }

    if (beam_pos[0] >= X(137+15)) {
      SpriteUpdatePos(&side_beam_l, 0, 0);
      SpriteUpdatePos(&side_beam_r, 0, 0);
      SpriteUpdatePos(&mid_beam, 0, 0);
      SpriteUpdatePos(&cock, 0, 0);
      phase = ESCAPE;
    }
  }
}

static void Escape(void) {
  Area2D clear_area = {109, UFO_H + ufo_pos - 2, UFO_W, 2};

  BitmapClearArea(screen[0], &clear_area);
  BitmapClearArea(screen[1], &clear_area);

  ufo_pos -= 2;
  DrawUfo(screen[0]);
  DrawUfo(screen[1]);

  if (ufo_pos < -37) {
    phase = FADE_OUT;
  }
}

static void FadeOut(void) {
  static short i = 0;
  static unsigned short fadeout_gradient[6] = {
    0x034,0x023,0x022,0x012,0x011,0x000
  };
  CopperStop();

  if (frameCount % 2 == 0) {
    SetColor(0, fadeout_gradient[i]);
    ++i;
  }

  if (i == 6) {
    phase = END;
  }
}


static CopListT *MakeCopperList(void) {
  short i = 0;

  cp = NewCopList(1100);
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);
  sprptr = CopSetupSprites(cp);

  CopWait(cp, 0, 0);
  CopLoadColors(cp, abduction_colors, 0);
  CopSetColor(cp, 0, 0x000);
  for (i = 0; i < HEIGHT; ++i) {
    CopWaitSafe(cp, Y(i), HP(0));
    CopSetColor(cp, 0, 0x034);
    if (i == 65) {
      ring_pal = CopLoadColors(cp, ring_colors, 0);
    }
    CopWaitSafe(cp, Y(i), HP(290));
    CopSetColor(cp, 0, 0x000);
  }

  return CopListFinish(cp);
}

static void Init(void) {
  TimeWarp(abduction_start);

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  LoadColors(pal, 0);

  EnableDMA(DMAF_BLITTER);

  (void)DrawBackground;
  DrawBackground(screen[0]);
  DrawBackground(screen[1]);
  DrawUfo(screen[0]);
  DrawUfo(screen[1]);

  cp = MakeCopperList();
  CopListActivate(cp);

  custom->bplcon2 = BPLCON2_PF2PRI;

  CopInsSetSprite(&sprptr[2], &cock);
  CopInsSetSprite(&sprptr[4], &mid_beam);
  CopInsSetSprite(&sprptr[6], &side_beam_l);
  CopInsSetSprite(&sprptr[7], &side_beam_r);

  SpriteUpdatePos(&cock,        X(152), Y(cock_pos));
  SpriteUpdatePos(&mid_beam,    X(152), Y(56));
  SpriteUpdatePos(&side_beam_l, X(137), Y(56));
  SpriteUpdatePos(&side_beam_r, X(167), Y(56));

  LoadColors(pal, 0);

  EnableDMA(DMAF_RASTER | DMAF_SPRITE);
}

static void Kill(void) {
  CopperStop();
  BlitterStop();
  ResetSprites();

  DeleteCopList(cp);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

PROFILE(Abduction);

#define TALK_IN   (abduction_start + (abduction_end/5))
#define SING_IN   (abduction_start + 2*(abduction_end/5))
#define NAKED_IN  (abduction_start + 3*(abduction_end/5))
#define NAKED_OUT (abduction_start + 4*(abduction_end/5))

static void Render(void) {
  static short idx = 0;
  (void)idx;
  ProfilerStart(Abduction);
  {

    if (frameCount >= TALK_IN) {
      if (idx <= 15) {
        BlitterFadeIn(screen[0]->planes, 2, idx, 160, 6, _talk_bpl, talk.bytesPerRow);
        BlitterFadeIn(screen[1]->planes, 2, idx, 160, 6, _talk_bpl, talk.bytesPerRow);
        BlitterFadeIn(screen[0]->planes, 3, idx, 160, 6, _talk_bpl, talk.bytesPerRow);
        BlitterFadeIn(screen[1]->planes, 3, idx, 160, 6, _talk_bpl, talk.bytesPerRow);
        ++idx;
      }
    }
    if (frameCount >= SING_IN) {
      if (idx <= 15+16) {
        BlitterFadeIn(screen[0]->planes,  2, idx-16, 128, 24, _sing_bpl, sing.bytesPerRow);
        BlitterFadeIn(screen[1]->planes,  2, idx-16, 128, 24, _sing_bpl, sing.bytesPerRow);
        BlitterFadeIn(screen[0]->planes,  3, idx-16, 128, 24, _sing_bpl, sing.bytesPerRow);
        BlitterFadeIn(screen[1]->planes,  3, idx-16, 128, 24, _sing_bpl, sing.bytesPerRow);
      
        BlitterFadeOut(screen[0]->planes, 2, idx-16, 160, 6, _talk_bpl, talk.bytesPerRow);
        BlitterFadeOut(screen[1]->planes, 2, idx-16, 160, 6, _talk_bpl, talk.bytesPerRow);
        BlitterFadeOut(screen[0]->planes, 3, idx-16, 160, 6, _talk_bpl, talk.bytesPerRow);
        BlitterFadeOut(screen[1]->planes, 3, idx-16, 160, 6, _talk_bpl, talk.bytesPerRow);
        ++idx;
      }
    }
    if (frameCount >= NAKED_IN) {
      if (idx <= 15+32) {
        BlitterFadeIn(screen[0]->planes,  2, idx-32, 64, 2, _naked_bpl, naked.bytesPerRow);
        BlitterFadeIn(screen[1]->planes,  2, idx-32, 64, 2, _naked_bpl, naked.bytesPerRow);
        BlitterFadeIn(screen[0]->planes,  3, idx-32, 64, 2, _naked_bpl, naked.bytesPerRow);
        BlitterFadeIn(screen[1]->planes,  3, idx-32, 64, 2, _naked_bpl, naked.bytesPerRow);

        BlitterFadeOut(screen[0]->planes, 2, idx-32, 128, 24, _sing_bpl, sing.bytesPerRow);
        BlitterFadeOut(screen[1]->planes, 2, idx-32, 128, 24, _sing_bpl, sing.bytesPerRow);
        BlitterFadeOut(screen[0]->planes, 3, idx-32, 128, 24, _sing_bpl, sing.bytesPerRow);
        BlitterFadeOut(screen[1]->planes, 3, idx-32, 128, 24, _sing_bpl, sing.bytesPerRow);
        ++idx;
      }
    }
    if (frameCount >= NAKED_OUT) {
      if (idx <= 15+48) {
        BlitterFadeOut(screen[0]->planes, 2, idx-48, 64, 2, _naked_bpl, naked.bytesPerRow);
        BlitterFadeOut(screen[1]->planes, 2, idx-48, 64, 2, _naked_bpl, naked.bytesPerRow);
        BlitterFadeOut(screen[0]->planes, 3, idx-48, 64, 2, _naked_bpl, naked.bytesPerRow);
        BlitterFadeOut(screen[1]->planes, 3, idx-48, 64, 2, _naked_bpl, naked.bytesPerRow);
        ++idx;
      }
    }

    // Music speeds up, cock speeds up
    if (frameCount >= abduction_start + (abduction_end / 2)) {
      cock_speed = 1;
    }

    if ((phase == ABDUCT || phase == RETRACT_BEAM) && frameCount % 4 == 0) {
      DrawUfo(screen[0]);
      DrawUfo(screen[1]);
    }

    switch (phase) {
      case FADEIN:
        phase = ABDUCT;
        break;

      case ABDUCT:
        Abduct();
        break;

      case RETRACT_BEAM:
        RetractBeam();
        break;

      case ESCAPE:
        Escape();
        break;

      case FADE_OUT:
        FadeOut();
        break;

      case END:
        break;
    }

    BitmapCopyArea(screen[active], 128, 230, &bkg, &((Area2D){128, 230, 64, 25}));
  }
  ProfilerStop(Abduction);

  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Abduction, NULL, NULL, Init, Kill, Render, NULL);
