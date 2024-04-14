#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <sprite.h>
#include <system/memory.h>
#include <system/interrupt.h>
#include <common.h>


#include "data/ground.c"
#include "data/ground2.c"

#include "data/tree1.c"
#include "data/tree2.c"
#include "data/tree3.c"

#include "data/moonghost.c"


#define WIDTH 320
#define HEIGHT 97  // 1 line for trees + 6 layers * 16 lines for ground
#define DEPTH 4
#define GROUND_HEIGHT 16


static BitmapT *screen[2];
static CopInsPairT *bplptr;
static CopListT *cp;
static short active = 0;


static __data_chip u_short treeTab[6][24] = {
    { // 1st LAYER (closest)
      0x000F, 0xFFFF, 0xFFF0, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF,
      0xF000, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x00FF, 0xFFFF, 0xFFF0, 0x0000,
      0x0, 0x0, 0x0, 0x0,
    },
    { // 2nd LAYER
      0x000F, 0xFFFF, 0xFFF0, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0FFF, 0xFFFF, 0xF000,
      0x0000, 0x0FFF, 0xFFFF, 0x0000, 0x0000,
      0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xF000,
      0x0, 0x0, 0x0, 0x0,
    },
    { // 3rd LAYER
      0x0000, 0x0000, 0x0FFF, 0xFFF0, 0x0000,
      0x0000, 0x00FF, 0xFFFF, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x000F,
      0xFFFF, 0xFF00, 0x0000, 0x000F, 0xFFFF,
      0x0, 0x0, 0x0, 0x0,
    },
    { // 4th LAYER
      0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000,
      0x00FF, 0xFFF0, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0FFF, 0xFF00,
      0x0000, 0x0FFF, 0xF000, 0x0000, 0x0000,
      0x0, 0x0, 0x0, 0x0,
    },
    { // 5th LAYER
      0x0000, 0x0000, 0x0FFF, 0xFFF0, 0x0000,
      0x000F, 0xFFF0, 0x0000, 0x0000, 0xFFFF,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0FFF, 0xF000,
      0x0, 0x0, 0x0, 0x0,
    },
    { // 6th LAYER
      0x00FF, 0xFF00, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0FFF, 0xFF00,
      0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0FFF, 0xF000, 0x0000,
      0x0, 0x0, 0x0, 0x0,
    },
};

static short branchesPos[4][2] = {
  // Calculate these values from treeTab in Init?
  {-4, 43},
  {112, 163},
  {248, 299},
};

static short groundLevel[6] = {
  // Number of lines counting from bottom
  // 180,  // 6th LAYER
  // 150,  // 5th LAYER
  // 120,  // 4th LAYER
  // 90,   // 3rd LAYER
  // 60,   // 2nd LAYER
  // 30,   // 1st LAYER (min. 16)
  160,  // 6th LAYER
  144,  // 5th LAYER
  122,  // 4th LAYER
  94,   // 3rd LAYER
  64,   // 2nd LAYER
  30,   // 1st LAYER (min. 16)
};

static short speed[6] = {
  // Trees move speed in pixels per frame
  // 1, 4, 2, 5, 3, 6
  2, 8, 4, 10, 6, 12
};

static short wordOffset[6] = {
  19, 19, 19, 19, 19, 19
};

static short bitOffset[6] = {
  0, 0, 0, 0, 0, 0
};

static short ghostPos[2] = {
  -32, 107
};

static bool ghostUp = false;


/* MOVEMENTS */
static void MoveTrees(short layer, u_short bits) {
  short i;
  u_short dong = treeTab[layer][23] << (16 - bits);

  for (i = 23; i > 0; --i) {
    treeTab[layer][i] = (treeTab[layer][i] >> bits) | (treeTab[layer][i-1] << (16 - bits));
  }
  treeTab[layer][0] = (treeTab[layer][0] >> bits) | dong;
}

static void MoveSprites(void) {
  short i, j;

  /* Move branches */
  SpriteUpdatePos(&tree1[0],   X(branchesPos[0][0]), Y(-16));
  SpriteUpdatePos(&tree1[1],   X(branchesPos[0][1]), Y(-16));

  SpriteUpdatePos(&tree2[0],   X(branchesPos[1][0]), Y(-16));
  SpriteUpdatePos(&tree2[1],   X(branchesPos[1][1]), Y(-16));

  SpriteUpdatePos(&tree3[0],   X(branchesPos[2][0]), Y(-16));
  SpriteUpdatePos(&tree3[1],   X(branchesPos[2][1]), Y(-16));

  for (i = 0; i <= 2; ++i) {
    for (j = 0; j <= 1; ++j) {
      if (branchesPos[i][j] > 320) {
        branchesPos[i][j] = -63;
      }
      branchesPos[i][j]++;
    }
  }

  /* Move ghost */
  moonghost[0].sprdat->data[32][0] = SPRPOS(X(ghostPos[0]), Y(ghostPos[1]));
  moonghost[0].sprdat->data[32][1] = SPRCTL(X(ghostPos[0]), Y(ghostPos[1]), false, 49);
  moonghost[1].sprdat->data[32][0] = SPRPOS(X(ghostPos[0]+16), Y(ghostPos[1]));
  moonghost[1].sprdat->data[32][1] = SPRCTL(X(ghostPos[0]+16), Y(ghostPos[1]), false, 49);

  ghostPos[0] += 2;
  if (ghostUp) {
    ghostPos[1]--;
  } else {
    ghostPos[1]++;
  }
  if (ghostPos[0] > 320) {
    ghostPos[0] = -64;
  }
  if (ghostPos[1] >= 110 || ghostPos[1] <= 104) {
    ghostUp = !ghostUp;
  }
}

static void MoveForest(void) {
  short i = 0;

  for (i = 0; i < 6; ++i) {
    if (speed[i] == 0) {
      if (i == 0) {
        MoveSprites();
      }
      speed[i] = i + 1;
      MoveTrees(i, 1);

      bitOffset[i] += 1;
      if (bitOffset[i] >= 16) {
        bitOffset[i] = 0;
        wordOffset[i] -= 1;
      }
      if (wordOffset[i] < 0) {
        wordOffset[i] = 19;
      }
    } else {
      speed[i] -= 1;
    }
  }
}

/* BLITTER */
static void VerticalFill(short layer, short h) {
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
  custom->bltsize = ((h - 1) << 6) | 20;
}

static void DrawForest(void) {
  /* This function takes 112 raster lines */
  void *dst;
  void *srca;
  void *srcb;
  void *srcc;

  (void)MoveTrees;
  (void)speed;

  { /* PLAYFIELD 1 */
    /* PLAYFIELD 1 BITPLANE 0 */
    dst = screen[active]->planes[0];
    srca = treeTab[3];
    srcb = treeTab[4];
    srcc = treeTab[5];

    custom->bltamod = 0;
    custom->bltbmod = 0;
    custom->bltcmod = 0;
    custom->bltdmod = 0;

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = srca;
    custom->bltbpt = srcb;
    custom->bltcpt = srcc;
    custom->bltdpt = dst;

    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | (NABNC | NABC | ANBNC | ANBC | ABNC | ABC);
    custom->bltcon1 = 0;
    custom->bltsize = (1 << 6) | 20;

    WaitBlitter();

    /* PLAYFIELD 1 BITPLANE 1 */
    dst = screen[active]->planes[2];
    srca = treeTab[3];
    srcb = treeTab[4];
    srcc = treeTab[5];

    custom->bltamod = 0;
    custom->bltbmod = 0;
    custom->bltcmod = 0;
    custom->bltdmod = 0;

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = srca;
    custom->bltbpt = srcb;
    custom->bltcpt = srcc;
    custom->bltdpt = dst;

    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | (NANBC | NABNC | NABC);
    custom->bltcon1 = 0;
    custom->bltsize = (1 << 6) | 20;

    WaitBlitter();
  }

  { /* PLAYFIELD 2 */
    /* PLAYFIELD 2 BITPLANE 0 */
    dst = screen[active]->planes[1];
    srca = treeTab[0];
    srcb = treeTab[1];
    srcc = treeTab[2];

    custom->bltamod = 0;
    custom->bltbmod = 0;
    custom->bltcmod = 0;
    custom->bltdmod = 0;

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = srca;
    custom->bltbpt = srcb;
    custom->bltcpt = srcc;
    custom->bltdpt = dst;

    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | (NABNC | NABC | ANBNC | ANBC | ABNC | ABC);
    custom->bltcon1 = 0;
    custom->bltsize = (1 << 6) | 20;

    WaitBlitter();

    /* PLAYFIELD 2 BITPLANE 1 */
    dst = screen[active]->planes[3];
    srca = treeTab[0];
    srcb = treeTab[1];
    srcc = treeTab[2];

    custom->bltamod = 0;
    custom->bltbmod = 0;
    custom->bltcmod = 0;
    custom->bltdmod = 0;

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = srca;
    custom->bltbpt = srcb;
    custom->bltcpt = srcc;
    custom->bltdpt = dst;

    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | (NANBC | NABNC | NABC);
    custom->bltcon1 = 0;
    custom->bltsize = (1 << 6) | 20;

    WaitBlitter();
  }
}

static void DrawGround(void) {
  void *dst;
  void *srca;
  void *srcb;
  void *srcc;

  { /* 6th LAYER */
    dst = screen[active]->planes[2] + 40;
    srca = _ground_bpl + wordOffset[5];
    srcb = screen[active]->planes[0];

    custom->bltamod = 40;
    custom->bltbmod = -40;
    custom->bltdmod = 0;

    custom->bltcdat = -1;

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = srca;
    custom->bltbpt = srcb;
    custom->bltdpt = dst;

    custom->bltcon0 = (SRCA | SRCB | DEST) | (ANBC | ANBNC) | ASHIFT(bitOffset[5]);
    custom->bltcon1 = 0;
    custom->bltsize = (GROUND_HEIGHT << 6) | 20;

    WaitBlitter();
  }

  { /* 5th LAYER */
    dst = screen[active]->planes[0] + 40 + 16*40;
    srca = _ground_bpl + wordOffset[4];

    custom->bltamod = 40;
    custom->bltdmod = 0;

    custom->bltbdat = -1;
    custom->bltcdat = -1;

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = srca;
    custom->bltdpt = dst;

    custom->bltcon0 = (SRCA | DEST) | A_TO_D | ASHIFT(bitOffset[4]);
    custom->bltcon1 = 0;
    custom->bltsize = (GROUND_HEIGHT << 6) | 20;

    WaitBlitter();
  }

  { /* 4th LAYER */
    dst = screen[active]->planes[2] + 40 + 32*40;
    srca = _ground2_bpl + wordOffset[3];
    srcb = screen[active]->planes[0];
    srcc = screen[active]->planes[2];

    custom->bltamod = 40;
    custom->bltbmod = -40;
    custom->bltcmod = -40;
    custom->bltdmod = 0;

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = srca;
    custom->bltbpt = srcb;
    custom->bltcpt = srcc;
    custom->bltdpt = dst;

    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | (NABC | NANBC | NANBNC) | ASHIFT(bitOffset[3]);
    custom->bltcon1 = 0;
    custom->bltsize = (GROUND_HEIGHT << 6) | 20;

    WaitBlitter();
  }

  { /* 3rd LAYER */
    dst = screen[active]->planes[3] + 40 + 48*40;
    srca = _ground_bpl + wordOffset[2];
    srcb = screen[active]->planes[1];

    custom->bltamod = 40;
    custom->bltbmod = -40;
    custom->bltdmod = 0;

    custom->bltcdat = -1;

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = srca;
    custom->bltbpt = srcb;
    custom->bltdpt = dst;

    custom->bltcon0 = (SRCA | SRCB | DEST) | (ANBC | ANBNC) | ASHIFT(bitOffset[2]);
    custom->bltcon1 = 0;
    custom->bltsize = (GROUND_HEIGHT << 6) | 20;

    WaitBlitter();
  }

  { /* 2nd LAYER */
    dst = screen[active]->planes[1] + 40 + 64*40;
    srca = _ground2_bpl + wordOffset[1];

    custom->bltamod = 40;
    custom->bltdmod = 0;

    custom->bltbdat = -1;
    custom->bltcdat = -1;

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = srca;
    custom->bltdpt = dst;

    custom->bltcon0 = (SRCA | DEST) | A_TO_D | ASHIFT(bitOffset[1]);
    custom->bltcon1 = 0;
    custom->bltsize = (GROUND_HEIGHT << 6) | 20;

    WaitBlitter();
  }

  { /* 1st LAYER */
    dst = screen[active]->planes[3] + 40 + 80*40;
    srca = _ground_bpl + wordOffset[0];
    srcb = screen[active]->planes[1];
    srcc = screen[active]->planes[3];

    custom->bltamod = 40;
    custom->bltbmod = -40;
    custom->bltcmod = -40;
    custom->bltdmod = 0;

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = srca;
    custom->bltbpt = srcb;
    custom->bltcpt = srcc;
    custom->bltdpt = dst;

    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | (NABC | NANBC | NANBNC) | ASHIFT(bitOffset[0]);
    custom->bltcon1 = 0;
    custom->bltsize = (GROUND_HEIGHT << 6) | 20;

    // WaitBlitter();
  }
}

static void ClearBitplanes(void) {
  void *dst = screen[active]->planes[0];

  custom->bltdmod = 0;

  custom->bltdpt = dst;

  custom->bltcon0 = (DEST) | 0x0;
  custom->bltcon1 = 0;
  custom->bltsize = ((4 * HEIGHT) << 6) | 20;

  WaitBlitter();
}

/* SETUP */
static void SetupColors(void) {
  unsigned short colors[7] = {
    0x111, // 1st layer
    0x223,
    0x334,
    0x445,
    0x556,
    0x667,
    0x778, // background
    // 0x111, // 1st layer
    // 0x232,
    // 0x343,
    // 0x454,
    // 0x565,
    // 0x676,
    // 0x787, // background
  };

  // TREES
  // PLAYFIELD 1
  SetColor(8,  colors[6]); // BACKGROUND
  SetColor(9,  colors[0]); // 1st LAYER
  SetColor(10, colors[2]); // 3rd LAYER
  SetColor(11, colors[1]); // 2nd LAYER
  SetColor(12, 0xF0F); // UNUSED
  SetColor(13, 0xF0F); // UNUSED
  SetColor(14, 0xF0F); // UNUSED
  SetColor(15, 0xF0F); // UNUSED
  // PLAYFLIELD 2
  SetColor(0,  colors[6]); // BACKGROUND
  SetColor(1,  colors[3]); // 4th LAYER
  SetColor(2,  colors[5]); // 6th LAYER
  SetColor(3,  colors[4]); // 5th LAYER
  SetColor(4,  0xF0F); // UNUSED
  SetColor(5,  0xF0F); // UNUSED
  SetColor(6,  0xF0F); // UNUSED
  SetColor(7,  0xF0F); // UNUSED

  // SPRITES
  // SPRITES 0-1
  SetColor(17, 0x111);
  SetColor(18, 0x031);
  SetColor(19, 0x999);
  // SPRITES 2-3
  SetColor(21, 0x111);
  SetColor(22, 0x920);
  SetColor(23, 0x031);
  // SPRITES 4-5
  SetColor(25, 0x111);
  SetColor(26, 0x920);
  SetColor(27, 0x031);
  // SPRITES 6-7
  SetColor(29, 0x666);
  SetColor(30, 0x999);
  SetColor(31, 0xDDD);
}

static void SetupSprites(void) {
  CopInsPairT *sprptr;
  sprptr = CopSetupSprites(cp);

  CopInsSetSprite(&sprptr[0], &tree1[0]);
  CopInsSetSprite(&sprptr[1], &tree1[1]);

  CopInsSetSprite(&sprptr[2], &tree2[0]);
  CopInsSetSprite(&sprptr[3], &tree2[1]);

  CopInsSetSprite(&sprptr[4], &tree3[0]);
  CopInsSetSprite(&sprptr[5], &tree3[1]);

  CopInsSetSprite(&sprptr[6], &moonghost[0]);
  CopInsSetSprite(&sprptr[7], &moonghost[1]);

  SpriteUpdatePos(&tree1[0],   X(branchesPos[0][0]), Y(-16));
  SpriteUpdatePos(&tree1[1],   X(branchesPos[0][1]), Y(-16));

  SpriteUpdatePos(&tree2[0],   X(branchesPos[1][0]), Y(-16));
  SpriteUpdatePos(&tree2[1],   X(branchesPos[1][1]), Y(-16));

  SpriteUpdatePos(&tree3[0],   X(branchesPos[2][0]), Y(-16));
  SpriteUpdatePos(&tree3[1],   X(branchesPos[2][1]), Y(-16));

  SpriteUpdatePos(&moonghost[0], X(16), Y(16));
  SpriteUpdatePos(&moonghost[1], X(32), Y(16));
}

static CopListT *MakeCopperList(void) {
  short i;
  unsigned short backgroundGradient[12] = {
    0x222,
    0x222,
    0x223,
    0x223,
    0x233,
    0x334,
    0x334,
    0x435,
    0x445,
    0x446,
    0x456,
    0x557
  };

  cp = NewCopList(1200);
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);

  // Sprites
  SetupSprites();

  // Moon behind trees
  CopWait(cp, 0, 0);
  CopMove16(cp, bplcon2, BPLCON2_PF2PRI | BPLCON2_PF1P0 | BPLCON2_PF1P1 | BPLCON2_PF2P0 | BPLCON2_PF2P1);

  // Duplicate lines
  CopWaitSafe(cp, Y(0), 0);
  CopMove16(cp, bpl1mod, -40);
  CopMove16(cp, bpl2mod, -40);

  // Ghost between playfields
  CopWait(cp, Y(80), 0);
  CopMove16(cp, bplcon2, BPLCON2_PF2PRI | BPLCON2_PF2P0 | BPLCON2_PF2P1 | BPLCON2_PF1P2);

  (void)backgroundGradient;
  // for (i = 0; i < 12; ++i) {
  //   CopWait(cp, Y(i * 8), 0);
  //   CopSetColor(cp, 0, backgroundGradient[i]);
  // }

  for (i = 0; i < 6; ++i) {
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

  moonghost[0].height = 32;
  moonghost[1].height = 32;
  moonghost[0].sprdat->ctl = SPRCTL(0, 0, false, 32);
  moonghost[1].sprdat->ctl = SPRCTL(0, 0, false, 32);

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
    ClearBitplanes();
    DrawForest();
    DrawGround();
    MoveForest();

    VerticalFill(0, HEIGHT);
    VerticalFill(1, HEIGHT);
    VerticalFill(2, HEIGHT-64);
    VerticalFill(3, HEIGHT-16);

    ITER(i, 0, DEPTH - 1, CopInsSet32(&bplptr[i], screen[active]->planes[i]));
  }
  ProfilerStop(Forest);

  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Forest, NULL, NULL, Init, Kill, Render, NULL);
