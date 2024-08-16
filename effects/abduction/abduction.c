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
#include "data/bkg.c"


#define WIDTH 320
#define HEIGHT 256
#define DEPTH 4

#define RING_W 56
#define RING_H 16

#define UFO_W 104
#define UFO_H 39


// typedef enum phase {
//   ABDUCT,
//   RETRACT_BEAM,
//   ESCAPE
// } phaseE;


static BitmapT *screen[2];
static CopInsPairT *bplptr;
static CopInsPairT *sprptr;
static CopListT *cp;
static CopInsT *beam_pal_cp;

// static phaseE phase = ABDUCT;
static short active = 0;
static short counter = 0;
static short active_pal = 0;
static short coq_pos = 255-24;
static short beam_pal[8][3] = {
  {0x55F, 0x77F, 0xBBF},
  {0x66F, 0x88F, 0xCCF},
  {0x77F, 0x99F, 0xDDF},
  {0x88F, 0xAAF, 0xEEF},
  {0x99F, 0xBBF, 0xFFF},
  {0x88F, 0xAAF, 0xEEF},
  {0x77F, 0x99F, 0xDDF},
  {0x66F, 0x88F, 0xCCF},
};
Area2D bkg_area = {0, 0, WIDTH, HEIGHT};


static void DrawUfo(void) {
  Area2D ufo_area = {0, 0, UFO_W, UFO_H};

  BitmapCopyArea(screen[active], 108, 25, &ufo, &ufo_area);
}

static void DrawRing(short height) {
  Area2D ring_area = {0, 0, RING_W, RING_H};

  if (height < 64) {
    ring_area.y = 64 - height;
    ring_area.h = RING_H - (64 - height);
    height = 64;
  }

  if (height + RING_H >= HEIGHT - 16) {
    ring_area.h = HEIGHT - height - 16;
  }

  BitmapCopyArea(screen[active], 133, height, &ring, &ring_area);
}

static void SwitchBeamPal(void) {
  CopInsT *ins = beam_pal_cp;

  CopInsSet16(ins++, beam_pal[active_pal][0]);
  CopInsSet16(ins++, beam_pal[active_pal][1]);
  CopInsSet16(ins++, beam_pal[active_pal][2]);
}

static void Abduct(void) {
  static short mod = 1;
  short i = 0;

  for (i = 64; i <= 256 - 16; i += RING_H) {
    DrawRing(i - mod);
  }
  
  if (counter % 2 == 0) {
    ++mod;
    if (mod >= RING_H) {
      mod = 1;
    }
  }

  if (counter % 3 == 0) {
    --coq_pos;
    if (coq_pos <= 32) {
      coq_pos = 256;
    }
    SpriteUpdatePos(&coq,  X(152), Y(coq_pos));
  }

  if (counter % 6 == 0) {
    SwitchBeamPal();
    active_pal = (active_pal + 1) % 8;
  }

  DrawUfo();
}


static CopListT *MakeCopperList(void) {
  cp = NewCopList(128);
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);

  sprptr = CopSetupSprites(cp);

  CopInsSetSprite(&sprptr[0], &coq);

  CopInsSetSprite(&sprptr[2], &beam[0]);
  CopInsSetSprite(&sprptr[3], &beam[1]);

  SpriteUpdatePos(&coq,  X(152), Y(coq_pos));
  SpriteUpdatePos(&beam[0], X(144), Y(56));
  SpriteUpdatePos(&beam[1], X(160), Y(56));

  beam_pal_cp = CopLoadColors(cp, beam_pal[0], 21);

  // CopWait(cp, Y(0), X(0));
  // CopSetColor(cp, 1, 0xAAA);
  // CopSetColor(cp, 2, 0x0F0);

  // CopWait(cp, Y(63), X(64));
  // CopSetColor(cp, 1, 0xDDF);
  // CopSetColor(cp, 2, 0x66F);

  return cp;
}

static void Init(void) {
  EnableDMA(DMAF_BLITTER | DMAF_RASTER | DMAF_SPRITE);
  // EnableDMA(DMAF_BLITHOG);

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);

  SetColor(0,  0x034);
  SetColor(1,  0x000);
  SetColor(2,  0xFFF);
  SetColor(3,  0x9EF);
  SetColor(4,  0x6DF);
  SetColor(5,  0x3BE);
  SetColor(6,  0x29B);
  SetColor(7,  0x289);
  SetColor(8,  0x157);
  SetColor(9,  0x035);
  SetColor(10, 0x034);

  SetColor(11, 0xF0F);
  SetColor(12, 0xF0F);
  SetColor(13, 0xF0F);
  SetColor(14, 0xF0F);
  SetColor(15, 0xF0F);

  SetColor(16, 0x000);
  // Coq
  SetColor(17, 0x035);
  SetColor(18, 0x2AD);
  SetColor(19, 0x00F);
  // Beam
  SetColor(21, 0x88F);
  SetColor(22, 0xAAF);
  SetColor(23, 0xEEF); 

  cp = MakeCopperList();
  CopListActivate(cp);

  custom->bplcon2 = BPLCON2_PF2PRI;
}

static void Kill(void) {
  DisableDMA(DMAF_BLITTER | DMAF_RASTER | DMAF_SPRITE);
  // DisableDMA(DMAF_BLITHOG);

  DeleteCopList(cp);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

PROFILE(Abduction);

static void Render(void) {
  (void)DrawRing;
  ProfilerStart(Abduction);
  {
    ++counter;
    Abduct();
    BitmapCopyArea(screen[active], 0, 0, &bkg, &bkg_area);
    // switch (phase) {

    //   case ABDUCT:
    //     Abduct();
    //     break;

    //   default:
    //     break;
    // }
  }
  ProfilerStop(Abduction);

  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Abduction, NULL, NULL, Init, Kill, Render, NULL);