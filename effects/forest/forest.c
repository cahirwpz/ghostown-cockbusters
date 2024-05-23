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

#include "data/moonbatghost1.c"
#include "data/moonbatghost2.c"


#define WIDTH 320
#define HEIGHT 97  // 1 line for trees + 6 layers * 16 lines for ground
#define DEPTH 4
#define GROUND_HEIGHT 16


static BitmapT *screen[2];
static CopInsPairT *bplptr;
static CopInsPairT *sprptr;
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

static short speed[6] = {
  // Trees move speed in pixels per frame
  // 1, 4, 2, 5, 3, 6
  2, 8, 4, 10, 6, 12
};

static short groundWordOffset[6] = {
  19, 19, 19, 19, 19, 19
};

static short groundBitOffset[6] = {
  0, 0, 0, 0, 0, 0
};

static short batPos[2] = {
  320, 56
};

static short ghostPos[2] = {
  -32, 116
};

static bool ghostUp = false;


/* MOVEMENTS */
static void MoveTrees(short layer) {
  u_short* tab = treeTab[layer];
  u_short tail = (tab[23] & 1) * 0x8000;

  tab[23] = (tab[23] / 2) | ((tab[23-1] & 1) * 0x8000);
  tab[22] = (tab[22] / 2) | ((tab[22-1] & 1) * 0x8000);
  tab[21] = (tab[21] / 2) | ((tab[21-1] & 1) * 0x8000);
  tab[20] = (tab[20] / 2) | ((tab[20-1] & 1) * 0x8000);

  tab[19] = (tab[19] / 2) | ((tab[19-1] & 1) * 0x8000);
  tab[18] = (tab[18] / 2) | ((tab[18-1] & 1) * 0x8000);
  tab[17] = (tab[17] / 2) | ((tab[17-1] & 1) * 0x8000);
  tab[16] = (tab[16] / 2) | ((tab[16-1] & 1) * 0x8000);
  tab[15] = (tab[15] / 2) | ((tab[15-1] & 1) * 0x8000);
  tab[14] = (tab[14] / 2) | ((tab[14-1] & 1) * 0x8000);
  tab[13] = (tab[13] / 2) | ((tab[13-1] & 1) * 0x8000);
  tab[12] = (tab[12] / 2) | ((tab[12-1] & 1) * 0x8000);
  tab[11] = (tab[11] / 2) | ((tab[11-1] & 1) * 0x8000);
  tab[10] = (tab[10] / 2) | ((tab[10-1] & 1) * 0x8000);

  tab[9] = (tab[9] / 2) | ((tab[9-1] & 1) * 0x8000);
  tab[8] = (tab[8] / 2) | ((tab[8-1] & 1) * 0x8000);
  tab[7] = (tab[7] / 2) | ((tab[7-1] & 1) * 0x8000);
  tab[6] = (tab[6] / 2) | ((tab[6-1] & 1) * 0x8000);
  tab[5] = (tab[5] / 2) | ((tab[5-1] & 1) * 0x8000);
  tab[4] = (tab[4] / 2) | ((tab[4-1] & 1) * 0x8000);
  tab[3] = (tab[3] / 2) | ((tab[3-1] & 1) * 0x8000);
  tab[2] = (tab[2] / 2) | ((tab[2-1] & 1) * 0x8000);
  tab[1] = (tab[1] / 2) | ((tab[1-1] & 1) * 0x8000);

  tab[0] = (tab[0] / 2) | tail;
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

  // Moon Bat Ghost
  /* Move bat */
  moonbatghost1[0].sprdat->data[32][0] = SPRPOS(X(batPos[0]), Y(batPos[1]));
  moonbatghost1[0].sprdat->data[32][1] = SPRCTL(X(batPos[0]), Y(batPos[1]), false, 19);
  moonbatghost1[1].sprdat->data[32][0] = SPRPOS(X(batPos[0]+16), Y(batPos[1]));
  moonbatghost1[1].sprdat->data[32][1] = SPRCTL(X(batPos[0]+16), Y(batPos[1]), false, 19);
  moonbatghost2[0].sprdat->data[32][0] = SPRPOS(X(batPos[0]), Y(batPos[1]));
  moonbatghost2[0].sprdat->data[32][1] = SPRCTL(X(batPos[0]), Y(batPos[1]), false, 19);
  moonbatghost2[1].sprdat->data[32][0] = SPRPOS(X(batPos[0]+16), Y(batPos[1]));
  moonbatghost2[1].sprdat->data[32][1] = SPRCTL(X(batPos[0]+16), Y(batPos[1]), false, 19);

  /* Move ghost */
  moonbatghost1[0].sprdat->data[32+21][0] = SPRPOS(X(ghostPos[0]), Y(ghostPos[1]));
  moonbatghost1[0].sprdat->data[32+21][1] = SPRCTL(X(ghostPos[0]), Y(ghostPos[1]), false, 46);
  moonbatghost1[1].sprdat->data[32+21][0] = SPRPOS(X(ghostPos[0]+16), Y(ghostPos[1]));
  moonbatghost1[1].sprdat->data[32+21][1] = SPRCTL(X(ghostPos[0]+16), Y(ghostPos[1]), false, 46);
  moonbatghost2[0].sprdat->data[32+21][0] = SPRPOS(X(ghostPos[0]), Y(ghostPos[1]));
  moonbatghost2[0].sprdat->data[32+21][1] = SPRCTL(X(ghostPos[0]), Y(ghostPos[1]), false, 46);
  moonbatghost2[1].sprdat->data[32+21][0] = SPRPOS(X(ghostPos[0]+16), Y(ghostPos[1]));
  moonbatghost2[1].sprdat->data[32+21][1] = SPRCTL(X(ghostPos[0]+16), Y(ghostPos[1]), false, 46);

  batPos[0] -= 1;
  if (batPos[0] < -64) {
    batPos[0] = 360;
  }
  ghostPos[0] += 2;
  if (ghostUp) {
    CopInsSetSprite(&sprptr[6], &moonbatghost1[0]);
    CopInsSetSprite(&sprptr[7], &moonbatghost1[1]);
    ghostPos[1]--;
  } else {
    CopInsSetSprite(&sprptr[6], &moonbatghost2[0]);
    CopInsSetSprite(&sprptr[7], &moonbatghost2[1]);
    ghostPos[1]++;
  }
  if (ghostPos[0] > 320) {
    ghostPos[0] = -64;
  }
  if (ghostPos[1] >= 120 || ghostPos[1] <= 114) {
    ghostUp = !ghostUp;
  }
}

static void MoveForest(short l) {

  if (speed[l] == 0) {
    speed[l] = l + 1;
    MoveTrees(l);

    groundBitOffset[l] += 1;
    if (groundBitOffset[l] >= 16) {
      groundBitOffset[l] = 0;
      groundWordOffset[l] -= 1;
    }
    if (groundWordOffset[l] < 0) {
      groundWordOffset[l] = 19;
    }
  } else {
    speed[l] -= 1;
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
    custom->bltsize = (1 << 6) | 20;  // 160 cycles

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
    custom->bltsize = (1 << 6) | 20;  // 160 cycles

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
    custom->bltsize = (1 << 6) | 20;  // 160 cycles

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
    custom->bltsize = (1 << 6) | 20;  // 160 cycles

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
    srca = _ground_bpl + groundWordOffset[5];
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

    custom->bltcon0 = (SRCA | SRCB | DEST) | (ANBC | ANBNC) | ASHIFT(groundBitOffset[5]);
    custom->bltcon1 = 0;
    custom->bltsize = (GROUND_HEIGHT << 6) | 20;  // 1920 cycles

    MoveForest(5);
    WaitBlitter();
  }

  { /* 5th LAYER */
    dst = screen[active]->planes[0] + 40 + 16*40;
    srca = _ground_bpl + groundWordOffset[4];

    custom->bltamod = 40;
    custom->bltdmod = 0;

    custom->bltbdat = -1;
    custom->bltcdat = -1;

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = srca;
    custom->bltdpt = dst;

    custom->bltcon0 = (SRCA | DEST) | A_TO_D | ASHIFT(groundBitOffset[4]);
    custom->bltcon1 = 0;
    custom->bltsize = (GROUND_HEIGHT << 6) | 20;  // 1280 cycles

    MoveForest(4);
    WaitBlitter();
  }

  { /* 4th LAYER */
    dst = screen[active]->planes[2] + 40 + 32*40;
    srca = _ground2_bpl + groundWordOffset[3];
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

    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | (NABC | NANBC | NANBNC) | ASHIFT(groundBitOffset[3]);
    custom->bltcon1 = 0;
    custom->bltsize = (GROUND_HEIGHT << 6) | 20;  // 2560 cycles

    MoveForest(3);
    WaitBlitter();
  }

  { /* 3rd LAYER */
    dst = screen[active]->planes[3] + 40 + 48*40;
    srca = _ground_bpl + groundWordOffset[2];
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

    custom->bltcon0 = (SRCA | SRCB | DEST) | (ANBC | ANBNC) | ASHIFT(groundBitOffset[2]);
    custom->bltcon1 = 0;
    custom->bltsize = (GROUND_HEIGHT << 6) | 20;  // 1920 cycles

    MoveForest(2);
    WaitBlitter();
  }

  { /* 2nd LAYER */
    dst = screen[active]->planes[1] + 40 + 64*40;
    srca = _ground2_bpl + groundWordOffset[1];

    custom->bltamod = 40;
    custom->bltdmod = 0;

    custom->bltbdat = -1;
    custom->bltcdat = -1;

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = srca;
    custom->bltdpt = dst;

    custom->bltcon0 = (SRCA | DEST) | A_TO_D | ASHIFT(groundBitOffset[1]);
    custom->bltcon1 = 0;
    custom->bltsize = (GROUND_HEIGHT << 6) | 20;  // 1280 cycles

    MoveForest(1);
    WaitBlitter();
  }

  { /* 1st LAYER */
    dst = screen[active]->planes[3] + 40 + 80*40;
    srca = _ground_bpl + groundWordOffset[0];
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

    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | (NABC | NANBC | NANBNC) | ASHIFT(groundBitOffset[0]);
    custom->bltcon1 = 0;
    custom->bltsize = (GROUND_HEIGHT << 6) | 20;  // 2560 cycles

    MoveForest(0);
    // WaitBlitter();
  }
}

static void ClearBitplanes(void) {
  void *dst = screen[active]->planes[0] + 40;

  custom->bltdmod = 0;

  custom->bltdpt = dst;

  custom->bltcon0 = (DEST) | 0x0;
  custom->bltcon1 = 0;
  custom->bltsize = (GROUND_HEIGHT << 6) | 20;

  WaitBlitter();

  dst = screen[active]->planes[2] + 40 + 16*40;

  custom->bltdmod = 0;

  custom->bltdpt = dst;

  custom->bltcon0 = (DEST) | 0x0;
  custom->bltcon1 = 0;
  custom->bltsize = (GROUND_HEIGHT << 6) | 20;

  WaitBlitter();

  dst = screen[active]->planes[1] + 40;

  custom->bltdmod = 0;

  custom->bltdpt = dst;

  custom->bltcon0 = (DEST) | 0x0;
  custom->bltcon1 = 0;
  custom->bltsize = ((GROUND_HEIGHT*4) << 6) | 20;

  WaitBlitter();

  dst = screen[active]->planes[3] + 40;

  custom->bltdmod = 0;

  custom->bltdpt = dst;

  custom->bltcon0 = (DEST) | 0x0;
  custom->bltcon1 = 0;
  custom->bltsize = ((GROUND_HEIGHT*5) << 6) | 20;

  if (speed[0] == 0) {
    MoveSprites();
  }
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
  // SPRITES 0-1  (tree 1)
  SetColor(17, 0x111);
  SetColor(18, 0x031);
  SetColor(19, 0x999);
  // SPRITES 2-3  (tree 2)
  SetColor(21, 0x111);
  SetColor(22, 0x920);
  SetColor(23, 0x031);
  // SPRITES 4-5  (tree 3)
  SetColor(25, 0x111);
  SetColor(26, 0x920);
  SetColor(27, 0x031);
  // SPRITES 6-7  (moon and ghost)
  SetColor(29, 0x444);
  SetColor(30, 0x222);
  SetColor(31, 0xBBB);
}

static void SetupSprites(void) {
  // CopInsPairT *sprptr;
  sprptr = CopSetupSprites(cp);

  CopInsSetSprite(&sprptr[0], &tree1[0]);
  CopInsSetSprite(&sprptr[1], &tree1[1]);

  CopInsSetSprite(&sprptr[2], &tree2[0]);
  CopInsSetSprite(&sprptr[3], &tree2[1]);

  CopInsSetSprite(&sprptr[4], &tree3[0]);
  CopInsSetSprite(&sprptr[5], &tree3[1]);

  CopInsSetSprite(&sprptr[6], &moonbatghost1[0]);
  CopInsSetSprite(&sprptr[7], &moonbatghost1[1]);

  SpriteUpdatePos(&tree1[0],   X(branchesPos[0][0]), Y(-16));
  SpriteUpdatePos(&tree1[1],   X(branchesPos[0][1]), Y(-16));

  SpriteUpdatePos(&tree2[0],   X(branchesPos[1][0]), Y(-16));
  SpriteUpdatePos(&tree2[1],   X(branchesPos[1][1]), Y(-16));

  SpriteUpdatePos(&tree3[0],   X(branchesPos[2][0]), Y(-16));
  SpriteUpdatePos(&tree3[1],   X(branchesPos[2][1]), Y(-16));

  SpriteUpdatePos(&moonbatghost1[0], X(16), Y(16));
  SpriteUpdatePos(&moonbatghost1[1], X(32), Y(16));
  SpriteUpdatePos(&moonbatghost2[0], X(16), Y(16));
  SpriteUpdatePos(&moonbatghost2[1], X(32), Y(16));
}

static CopListT *MakeCopperList(void) {
  short i;
  unsigned short backgroundGradient[12] = {
    0x222, 0x222, 0x223, 0x223,
    0x233, 0x334, 0x334, 0x435,
    0x445, 0x446, 0x456, 0x557
  };
  static short groundLevel[6] = {
    160,  // 6th LAYER (changing these values may brake copper list)
    144,  // 5th LAYER
    122,  // 4th LAYER
    94,   // 3rd LAYER
    64,   // 2nd LAYER
    30,   // 1st LAYER (min. 16)
  };

  cp = NewCopList(1200);
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);

  // Sprites
  SetupSprites();

  // Moon behind trees
  CopWait(cp, 0, 0);
  CopMove16(cp, bplcon2, BPLCON2_PF2PRI | BPLCON2_PF1P0 | BPLCON2_PF1P1 | BPLCON2_PF2P0 | BPLCON2_PF2P1);

  // Moon colors
  CopSetColor(cp, 29, 0x444);
  CopSetColor(cp, 30, 0x777);
  CopSetColor(cp, 31, 0xBBB);

  // Duplicate lines
  CopWaitSafe(cp, Y(0), 0);
  CopMove16(cp, bpl1mod, -40);
  CopMove16(cp, bpl2mod, -40);

  // Background gradient
  for (i = 0; i < 12; ++i) {
    CopWait(cp, Y(i * 8), 0);
    CopSetColor(cp, 0, backgroundGradient[i]);
    // Bat Colors
    if (i * 8 == 48) {
      CopSetColor(cp, 29, 0x222);
      CopSetColor(cp, 30, 0x777);
      CopSetColor(cp, 31, 0x700);
    }
  }

  // Ghost colors
  CopSetColor(cp, 29, 0x666);
  CopSetColor(cp, 30, 0x999);
  CopSetColor(cp, 31, 0xDDD);

  // Ghost between playfields
  CopMove16(cp, bplcon2, BPLCON2_PF2PRI | BPLCON2_PF2P0 | BPLCON2_PF2P1 | BPLCON2_PF1P2);

  for (i = 0; i < 6; ++i) {
    CopWaitSafe(cp, Y(256 - groundLevel[i]), 0);
    CopMove16(cp, bpl1mod, 0);
    CopMove16(cp, bpl2mod, 0);
    CopWaitSafe(cp, Y(256 + GROUND_HEIGHT - groundLevel[i]), 0);
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

  moonbatghost1[0].height = 32;
  moonbatghost1[1].height = 32;
  moonbatghost1[0].sprdat->ctl = SPRCTL(0, 0, false, 32);
  moonbatghost1[1].sprdat->ctl = SPRCTL(0, 0, false, 32);

  moonbatghost2[0].height = 32;
  moonbatghost2[1].height = 32;
  moonbatghost2[0].sprdat->ctl = SPRCTL(0, 0, false, 32);
  moonbatghost2[1].sprdat->ctl = SPRCTL(0, 0, false, 32);

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
  (void)MoveForest;
  (void)DrawForest;
  (void)DrawGround;
  (void)ClearBitplanes;
  (void)MoveSprites;
  ProfilerStart(Forest);
  {
    ClearBitplanes();
    DrawForest();
    DrawGround();

    VerticalFill(0, HEIGHT - GROUND_HEIGHT*2);
    VerticalFill(1, HEIGHT);
    VerticalFill(2, HEIGHT - GROUND_HEIGHT*4);
    VerticalFill(3, HEIGHT - GROUND_HEIGHT);

    ITER(i, 0, DEPTH - 1, CopInsSet32(&bplptr[i], screen[active]->planes[i]));
  }
  ProfilerStop(Forest);

  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Forest, NULL, NULL, Init, Kill, Render, NULL);
