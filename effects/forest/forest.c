#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <sprite.h>
#include <system/memory.h>
#include <system/interrupt.h>
#include <common.h>


#include "data/ground.c"
#include "../twister-rgb/data/twister-left.c"
#include "../twister-rgb/data/twister-right.c"


#define WIDTH 320
#define HEIGHT 97  // 1 line for trees + 6 layers * 16 lines for ground
#define DEPTH 6


static BitmapT *screen[2];
static CopInsPairT *bplptr;
static CopListT *cp;
static short active = 0;


static u_short treeTab[6][20] = {
    { // 6th LAYER
      0x00FF, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x000F, 0xF000,
      0x0000, 0xFF00, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x00FF, 0x0000, 0x0000,
    },
    { // 3rd LAYER
      0x0000, 0x0000, 0x00FF, 0xFF00, 0x0000,
      0x0000, 0x0000, 0xFFF0, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
      0xFFFF, 0x0000, 0x0000, 0x0000, 0x00FF,
    },
    { // 5th LAYER
      0x0000, 0x0000, 0x0FFF, 0xF000, 0x0000,
      0x0000, 0xFF00, 0x0000, 0x0000, 0xFFF0,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x00FF, 0x0000,
    },
    { // 2nd LAYER
      0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000,
      0x0000, 0x0FFF, 0xF000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0xFFFF, 0xF000,
    },
    { // 4th LAYER
      0x0000, 0x0000, 0x0000, 0x0000, 0x0FFF,
      0x0FFF, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0FF0, 0x0000,
      0x0000, 0x0FFF, 0x0000, 0x0000, 0x0000,
    },
    { // 1st LAYER
      0x0000, 0x0000, 0xFFFF, 0xF000, 0x0000,
      0x0000, 0xFFFF, 0xF000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x00FF, 0xFF00, 0x0000,
      0x0000, 0x0000, 0x0FFF, 0xFFF0, 0x0000,
    },
};

static u_short groundLevel[6] = {
  // Number of lines counting from bottom
  180,  // 6th LAYER
  150,  // 5th LAYER
  120,  // 4th LAYER
  90,   // 3rd LAYER
  60,   // 2nd LAYER
  30,   // 1st LAYER (min. 16)
};

static short speed[6] = {
  // Trees move speed in pixels per frame
  1, 4, 2, 5, 3, 6
};


static void MoveTrees(short layer, u_short bits) {
  short i;
  u_short dong = treeTab[layer][0] >> (16 - bits);

  for (i = 0; i < 19; ++i) {
    treeTab[layer][i] = (treeTab[layer][i] << bits) | (treeTab[layer][i+1] >> (16 - bits));
  }
  treeTab[layer][19] = (treeTab[layer][19] << bits) | dong;
}

static void DrawForest(void) {
  /* This function takes 112 raster lines */
  short i, layer;
  short* ptr;

  for (layer = 0; layer < 6; ++layer) {
    ptr = screen[active]->planes[layer];

    /* Clear first line */
    for (i = 0; i < WIDTH/16; ++i) {
      ptr[i] = 0x0;
    }

    /* Fill first line */
    for (i = 0; i < WIDTH/16; ++i) {
      ptr[i] = treeTab[layer][i];
    }

    /* Move */
    MoveTrees(layer, speed[layer]);
  }
}

static void DrawGround(void) {
  // TODO: Move ground with trees
  void *dst = screen[active]->planes[0];
  void *src = grass;


  custom->bltamod = 0;
  custom->bltdmod = 0;

  custom->bltbdat = -1;
  custom->bltcdat = -1;

  custom->bltafwm = -1;
  custom->bltalwm = -1;

  custom->bltapt = src;
  custom->bltdpt = dst;

  custom->bltcon0 = (SRCA | DEST) | A_TO_D;
  custom->bltcon1 = 0;
  custom->bltsize = ((HEIGHT * 6) << 6) | 20;
}

static void VerticalFill(short layer) {
  void *srca = screen[active]->planes[layer];
  void *srcb = screen[active]->planes[layer] + WIDTH/8;
  void *dst  = screen[active]->planes[layer] + WIDTH/8;

  WaitBlitter();

  custom->bltamod = 0;
  custom->bltbmod = 0;
  custom->bltdmod = 0;

  custom->bltcdat = -1;

  custom->bltafwm = -1;
  custom->bltalwm = -1;

  custom->bltapt = srca;
  custom->bltbpt = srcb;
  custom->bltdpt = dst;

  custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | NABC | ANBC);
  custom->bltcon1 = 0;
  custom->bltsize = ((HEIGHT - 1) << 6) | 20;
}

static void SetupColors(void) {
  // TREES
  SetColor(0,  0x999); // BACKGROUND
  // PLAYFIELD 1
  SetColor(8,  0x999); // BACKGROUND
  SetColor(9,  0x333); // 3rd LAYER
  SetColor(10, 0x222); // 2nd LAYER
  SetColor(11, 0x222); // 2nd LAYER
  SetColor(12, 0x111); // 1st LAYER
  SetColor(13, 0x111); // 1st LAYER
  SetColor(14, 0x111); // 1st LAYER
  SetColor(15, 0x111); // 1st LAYER
  // PLAYFLIELD 2
  SetColor(1,  0x666); // 6th LAYER
  SetColor(2,  0x555); // 5th LAYER
  SetColor(3,  0x555); // 5th LAYER
  SetColor(4,  0x444); // 4th LAYER
  SetColor(5,  0x444); // 4th LAYER
  SetColor(6,  0x444); // 4th LAYER
  SetColor(7,  0x444); // 4th LAYER

  // SPRITES
  // SPRITES 1-2
  SetColor(17, 0xC44);
  SetColor(18, 0xC66);
  SetColor(19, 0xC88);
  // SPRITES 3-4
  SetColor(21, 0xC44);
  SetColor(22, 0xC66);
  SetColor(23, 0xC88);
  // SPRITES 5-6
  SetColor(25, 0xC44);
  SetColor(26, 0xC66);
  SetColor(27, 0xC88);
  // SPRITES 7-8
  SetColor(29, 0xC44);
  SetColor(30, 0xC66);
  SetColor(31, 0xC88);
}

static CopListT *MakeCopperList(void) {
  short i;
  CopInsPairT *sprptr;

  cp = NewCopList(1200);
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);

  // Sprites
  sprptr = CopSetupSprites(cp);

  CopInsSetSprite(&sprptr[4], &left[0]);
  CopInsSetSprite(&sprptr[5], &left[1]);
  CopInsSetSprite(&sprptr[6], &right[0]);
  CopInsSetSprite(&sprptr[7], &right[1]);

  SpriteUpdatePos(&left[0],  X(160-32), Y(0));
  SpriteUpdatePos(&left[1],  X(160-16), Y(0));
  SpriteUpdatePos(&right[0], X(160),    Y(0));
  SpriteUpdatePos(&right[1], X(160+16), Y(0));

  // Duplicate lines
  CopWaitSafe(cp, Y(0), 0);
  CopMove16(cp, bpl1mod, -40);
  CopMove16(cp, bpl2mod, -40);

  for (i = 0; i < DEPTH; ++i) {
    CopWaitSafe(cp, Y(256 - groundLevel[i]), 0);
    CopMove16(cp, bpl1mod, 0);
    CopMove16(cp, bpl2mod, 0);
    CopWaitSafe(cp, Y(272 - groundLevel[i]), 0);
    CopMove16(cp, bpl1mod, -40);
    CopMove16(cp, bpl2mod, -40);
  }

  CopListFinish(cp);

  return cp;
}

static void Init(void) {
  EnableDMA(DMAF_BLITTER);
  EnableDMA(DMAF_RASTER);
  EnableDMA(DMAF_SPRITE);

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  SetupPlayfield(MODE_DUALPF, DEPTH, X(0), Y(0), WIDTH, 256);

  // SPRITES BETWEEN PLAYFIELDS
  custom->bplcon2 = BPLCON2_PF2PRI | BPLCON2_PF1P2;

  SetupColors();

  cp = MakeCopperList();
  CopListActivate(cp);
}

static void Kill(void) {
  DisableDMA(DMAF_BLITTER | DMAF_RASTER | DMAF_SPRITE);

  DeleteCopList(cp);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

PROFILE(Forest);

static void Render(void) {
  ProfilerStart(Forest);
  {
    DrawGround();
    WaitBlitter();
    DrawForest();
    ITER(i, 0, DEPTH - 1, VerticalFill(i));
    ITER(i, 0, DEPTH - 1, CopInsSet32(&bplptr[i], screen[active]->planes[i]));
  }
  ProfilerStop(Forest);

  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Forest, NULL, NULL, Init, Kill, Render, NULL);
