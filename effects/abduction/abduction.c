#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <sprite.h>
#include <system/memory.h>
#include <system/interrupt.h>
#include <common.h>
#include <2d.h>

#include "data/ufo.c"
#include "data/ring.c"
#include "data/coq.c"
#include "data/beam.c"


#define WIDTH 320
#define HEIGHT 256
#define DEPTH 2



static BitmapT *screen[2];
static CopInsPairT *bplptr;
static CopInsPairT *sprptr;
static CopListT *cp;
static CopInsT *beam_pal_cp;
static short active = 0;

static short counter = 0;
static short active_pal = 0;
static short coq_pos = 220;
static short beam_pal[2][3] = {
  {0x88F, 0xAAF, 0xEEF},
  {0x99F, 0xBBF, 0xFFF},
};


static void DrawUfo(void) {
  Area2D ufo_area = {0, 0, 180, 64};

  BitmapCopyArea(screen[active], 70, 0, &ufo, &ufo_area);
}

static void DrawRing(short height) {
  Area2D ring_area = {0, 0, 128, 32};

  if (height < 64) {
    ring_area.y = 64 - height;
    ring_area.h = 32 - (64 - height);
    height = 64;
  }

  if (height + 32 >= HEIGHT) {
    ring_area.h = HEIGHT - height;
  }

  BitmapCopyArea(screen[active], 96, height, &ring, &ring_area);
}

static void LoadBeamPal(void) {
  CopInsT *ins = beam_pal_cp;

  CopInsSet16(ins++, beam_pal[active_pal][0]);
  CopInsSet16(ins++, beam_pal[active_pal][1]);
  CopInsSet16(ins++, beam_pal[active_pal][2]);
}


static CopListT *MakeCopperList(void) {
  cp = NewCopList(128);
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);

  sprptr = CopSetupSprites(cp);

  CopInsSetSprite(&sprptr[0], &coq[0]);
  CopInsSetSprite(&sprptr[1], &coq[1]);

  CopInsSetSprite(&sprptr[2], &beam[0]);
  CopInsSetSprite(&sprptr[3], &beam[1]);

  SpriteUpdatePos(&coq[0],  X(144), Y(coq_pos));
  SpriteUpdatePos(&coq[1],  X(160), Y(coq_pos));
  SpriteUpdatePos(&beam[0], X(144), Y(56));
  SpriteUpdatePos(&beam[1], X(160), Y(56));

  beam_pal_cp = CopLoadColors(cp, beam_pal[0], 21);

  CopWait(cp, Y(0), X(0));
  CopSetColor(cp, 1, 0xAAA);
  CopSetColor(cp, 2, 0x0F0);

  CopWait(cp, Y(63), X(64));
  CopSetColor(cp, 1, 0xDDF);
  CopSetColor(cp, 2, 0x66F);

  return cp;
}

static void Init(void) {
  EnableDMA(DMAF_BLITTER | DMAF_BLITHOG | DMAF_RASTER | DMAF_SPRITE);

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);

  SetColor(0, 0x000);
  SetColor(1, 0xAAA);
  SetColor(2, 0x0F0);
  SetColor(3, 0x88F);

  SetColor(16, 0x000);

  SetColor(17, 0x222);
  SetColor(18, 0x88F);
  SetColor(19, 0x222);

  // SetColor(21, 0x88F);
  // SetColor(22, 0xAAF);
  // SetColor(23, 0xEEF);

  cp = MakeCopperList();
  CopListActivate(cp);

  custom->bplcon2 = BPLCON2_PF2PRI;
}

static void Kill(void) {
  DisableDMA(DMAF_BLITTER | DMAF_BLITHOG | DMAF_RASTER | DMAF_SPRITE);

  DeleteCopList(cp);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

PROFILE(Abduction);

static void Render(void) {
  static short mod = 1;
  short i = 0;

  ProfilerStart(Abduction);
  {
    ++counter;
    for (i = 64; i <= 256; i += 32) {
      DrawRing(i - mod);
    }
    if (counter % 2 == 0) {
      ++mod;
      if (mod >= 32) {
        mod = 1;
      }
    }

    if (counter % 3 == 0) {
      --coq_pos;
      if (coq_pos < 32) {
        coq_pos = 256;
      }
      SpriteUpdatePos(&coq[0],  X(144), Y(coq_pos));
      SpriteUpdatePos(&coq[1],  X(160), Y(coq_pos));
      LoadBeamPal();
      active_pal ^= 1;
    }
    
    DrawUfo();
  }
  ProfilerStop(Abduction);

  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Abduction, NULL, NULL, Init, Kill, Render, NULL);