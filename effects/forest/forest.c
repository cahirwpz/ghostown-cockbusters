#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <system/memory.h>
#include <system/interrupt.h>
#include <common.h>


#include "data/grass.c"

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 5


static BitmapT *screen[2];
static CopListT *cp;
static short active = 0;
static CopInsPairT *bplptr;


static inline int fastrand(void) {
  static int m[2] = { 0x3E50B28C, 0xD461A7F9 };

  int a, b;

  // https://www.atari-forum.com/viewtopic.php?p=188000#p188000
  asm volatile("move.l (%2)+,%0\n"
               "move.l (%2),%1\n"
               "swap   %1\n"
               "add.l  %0,(%2)\n"
               "add.l  %1,-(%2)\n"
               : "=d" (a), "=d" (b)
               : "a" (m));
  
  return a;
}

static u_short treeTab[DEPTH][20] = {
    {0x000F, 0xF000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0xFF00, 0x0000, 0x0000,
     0x0000, 0x00FF, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x00FF, 0x0000, 0x0000,},

    {0x0000, 0x0000, 0x0000, 0x0FF0, 0x0000,
     0x0FFF, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0FFF, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},

    {0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF,
     0x0000, 0x0000, 0x0FFF, 0x0000, 0x0000,
     0x0000, 0x00FF, 0x0000, 0x0000, 0x0000},

    {0x00FF, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0FFF, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0xFFF0},

    {0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFF0,
     0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0FFF, 0xFFFF, 0xFFF0,
     0x0000, 0x0000, 0x0000, 0x0FFF, 0xFF00},
};


static void DrawForest(short layer) {
  short i, aux;
  static short count = 0;
  short* ptr = screen[active]->planes[layer];

  /* Clear first line */
  for (i = 0; i < WIDTH/16; ++i) {
    ptr[i] = 0x0;
  }

  /* Fill first line */
  for (i = 0; i < WIDTH/16; ++i) {
    ptr[i] = treeTab[layer][i];
  }

  (void)aux;
  (void)count;
  // if (++count > 50) {
  //   aux = treeTab[layer][0];
  //   for (i = 1; i < 20; ++i) {
  //     treeTab[layer][i-1] = treeTab[layer][i];
  //   }
  //   treeTab[layer][19] = aux;
  //   count = 0;
  // }
}

static void PlantGrass(short layer) {
  void *src = grass;
  void *dst = screen[active]->planes[layer] + WIDTH/8 * 96 + (960 * (layer+1));

  WaitBlitter();

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
  custom->bltsize = (16 << 6) | 20;
}

#define BLTSIZE (WIDTH*HEIGHT/16)

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
  custom->bltsize = (254 << 6) | 20;
}

static void Init(void) {
  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);

  EnableDMA(DMAF_BLITTER);
  EnableDMA(DMAF_RASTER);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);

  SetColor(0,  0x999); // BACKGROUND   0b00000

  SetColor(1,  0x666); // 5th LAYER    0b00001

  SetColor(2,  0x555); // 4th LAYER    0b00010
  SetColor(3,  0x555); // 4th LAYER    0b00011

  SetColor(4,  0x444); // 3th LAYER    0b00100
  SetColor(5,  0x444); // 3th LAYER    0b00101
  SetColor(6,  0x444); // 3th LAYER    0b00110
  SetColor(7,  0x444); // 3th LAYER    0b00111

  SetColor(8,  0x333); // 2nd LAYER    0b01000
  SetColor(9,  0x333); // 2nd LAYER    0b01001
  SetColor(10, 0x333); // 2nd LAYER    0b01010
  SetColor(11, 0x333); // 2nd LAYER    0b01011
  SetColor(12, 0x333); // 2nd LAYER    0b01100
  SetColor(13, 0x333); // 2nd LAYER    0b01101
  SetColor(14, 0x333); // 2nd LAYER    0b01110
  SetColor(15, 0x333); // 2nd LAYER    0b01111

  SetColor(16, 0x222); // 1st LAYER    0b10000
  SetColor(17, 0x222); // 1st LAYER    0b10001
  SetColor(18, 0x222); // 1st LAYER    0b10010
  SetColor(19, 0x222); // 1st LAYER    0b10011
  SetColor(20, 0x222); // 1st LAYER    0b10100
  SetColor(21, 0x222); // 1st LAYER    0b10101
  SetColor(22, 0x222); // 1st LAYER    0b10110
  SetColor(23, 0x222); // 1st LAYER    0b10111
  SetColor(24, 0x222); // 1st LAYER    0b11000
  SetColor(25, 0x222); // 1st LAYER    0b11001
  SetColor(26, 0x222); // 1st LAYER    0b11010
  SetColor(27, 0x222); // 1st LAYER    0b11011
  SetColor(28, 0x222); // 1st LAYER    0b11100
  SetColor(29, 0x222); // 1st LAYER    0b11101
  SetColor(30, 0x222); // 1st LAYER    0b11110
  SetColor(31, 0x222); // 1st LAYER    0b11111

  cp = NewCopList(100);
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);
  CopListFinish(cp);

  CopListActivate(cp);
}

static void Kill(void) {
  DisableDMA(DMAF_BLITTER | DMAF_RASTER);

  DeleteCopList(cp);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

PROFILE(Forest);

static void Render(void) {
  ProfilerStart(Forest);
  {
    ITER(i, 0, DEPTH - 1, DrawForest(i));
    ITER(i, 0, DEPTH - 1, PlantGrass(i));
    ITER(i, 0, DEPTH - 1, VerticalFill(i));
  }
  ProfilerStop(Forest);

  ITER(i, 0, DEPTH - 1, CopInsSet32(&bplptr[i], screen[active]->planes[i]));

  BitmapClear(screen[active^1]);
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Forest, NULL, NULL, Init, Kill, Render, NULL);
