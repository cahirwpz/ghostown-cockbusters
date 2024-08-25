#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <sprite.h>
#include <system/memory.h>
#include <system/interrupt.h>
#include <common.h>
#include <2d.h>
#include <color.h>

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
static BitmapT *screen[2];
static CopInsPairT *bplptr;
static CopInsPairT *sprptr;
static CopInsT *ring_pal;
static CopListT *cp;
static short active = 0;

static phaseE phase = FADEIN;
static short counter = 0;

static short ufo_idx = 0;
static short ufo_pos = 25;
static short cock_pos = 255-24;
static short beam_pos[2] = {X(137), X(167)};

static short active_pal = 0;
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
  [11] = 0xF0F,
  [12] = 0xF0F,
  [13] = 0xF0F,
  [14] = 0xF0F,
  [15] = 0xF0F,

  // Coq
  [16] = 0x000,
  [17] = 0x035,
  [18] = 0x2AD,
  [19] = 0x00F,
  // Beam
  [20] = 0x000,
  [21] = 0xCEF,
  [22] = 0xCEF,
  [23] = 0xCEF,

  [24] = 0x000,
  [25] = 0x2AD,
  [26] = 0x079,
  [27] = 0x046,

  [28] = 0x000,
  [29] = 0x000,
  [30] = 0x000,
  [31] = 0x000,
};


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
    SetColor(21 + i, beam_pal[active_pal][i]);
  }

  active_pal = (active_pal + 1) % 4;
}


static void Abduct(void) {
  static short mod = 1;
  short i = 0;

  for (i = 64+16; i <= 256 - 16; i += RING_H*2) {
    DrawRing(screen[active], i - mod);
  }

  if (counter % 1 == 0) {
    if (mod < RING_H*2 - 1) {
      ++mod;
    }
    else {
      mod = 0;
    }
  }

  if (counter % 2 == 0) {
    if (cock_pos > 32) {
      --cock_pos;
      SpriteUpdatePos(&cock,  X(152), Y(cock_pos));
    } else {
      phase = RETRACT_BEAM;
    }
  }

  if (counter % 8 == 0) {
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

  if (counter % 2 == 0) {
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

  if (counter % 2 == 0) {
    SetColor(0, fadeout_gradient[i]);
    ++i;
  }

  if (i == 6) {
    phase = END;
  }
}


static CopListT *MakeCopperList(void) {
  short i = 0;

  cp = NewCopList(2048);
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
  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  LoadColors(pal, 0);

  EnableDMA(DMAF_BLITTER);

  DrawBackground(screen[0]);
  DrawBackground(screen[1]);
  DrawUfo(screen[0]);
  DrawUfo(screen[1]);

  cp = MakeCopperList();
  // CopListActivate(cp);

  custom->bplcon2 = BPLCON2_PF2PRI;

  CopInsSetSprite(&sprptr[0], &cock);
  CopInsSetSprite(&sprptr[2], &mid_beam);
  CopInsSetSprite(&sprptr[4], &side_beam_l);
  CopInsSetSprite(&sprptr[5], &side_beam_r);

  SpriteUpdatePos(&cock,        X(152), Y(cock_pos));
  SpriteUpdatePos(&mid_beam,    X(152), Y(56));
  SpriteUpdatePos(&side_beam_l, X(137), Y(56));
  SpriteUpdatePos(&side_beam_r, X(167), Y(56));

  LoadColors(pal, 0);

  EnableDMA(DMAF_RASTER | DMAF_SPRITE);
}

static void Kill(void) {
  DisableDMA(DMAF_BLITTER | DMAF_RASTER | DMAF_SPRITE);

  DeleteCopList(cp);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

PROFILE(Abduction);

static void Render(void) {
  (void)pal;
  ProfilerStart(Abduction);
  {
    ++counter;

    if ((phase == ABDUCT || phase == RETRACT_BEAM) && counter % 4 == 0) {
      DrawUfo(screen[0]);
      DrawUfo(screen[1]);
    }

    switch (phase) {
      case FADEIN:
        CopListActivate(cp);
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