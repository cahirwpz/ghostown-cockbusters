#include <effect.h>
#include <copper.h>
#include <blitter.h>
#include <gfx.h>
#include <line.h>
#include <sprite.h>
#include <ptplayer.h>
#include <color.h>

#define _SYSTEM
#include <system/cia.h>

#include "data/loader.c"
#include "data/ufo.c"
#include "data/noaga.c"

extern u_char LoaderModule[];
extern u_char LoaderSamples[];

static __code BitmapT *screen;
static __code CopListT *cp;
static __code CopInsPairT *sprptr;

#define X1 99
#define Y1 230
#define X2 220
#define Y2 238

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 3

static bool show_noaga = false;

static void Init(void) {
  PtInstallCIA();
  PtInit(LoaderModule, LoaderSamples, 1);
  PtEnable = 1;

  screen = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  cp = NewCopList(40);

  CpuLineSetup(screen, 0);
  CpuLine(X1 - 1, Y1 - 2, X2 + 1, Y1 - 2);
  CpuLine(X1 - 1, Y2 + 1, X2 + 1, Y2 + 1);
  CpuLine(X1 - 2, Y1 - 1, X1 - 2, Y2 + 1);
  CpuLine(X2 + 2, Y1 - 1, X2 + 2, Y2 + 1);

  sprptr = CopSetupSprites(cp);
  CopInsSetSprite(&sprptr[0], &ufo[0]);
  CopInsSetSprite(&sprptr[1], &ufo[1]);
  CopInsSetSprite(&sprptr[2], &ufo[2]);

  CopInsSetSprite(&sprptr[4], &noaga[0]);
  CopInsSetSprite(&sprptr[5], &noaga[1]);
  CopInsSetSprite(&sprptr[6], &noaga[2]);
  CopInsSetSprite(&sprptr[7], &noaga[3]);

  #define NOAGA_X 144
  #define NOAGA_Y 12

  SpriteUpdatePos(&noaga[0], X(NOAGA_X),    Y(NOAGA_Y));
  SpriteUpdatePos(&noaga[1], X(NOAGA_X+16), Y(NOAGA_Y));
  SpriteUpdatePos(&noaga[2], X(NOAGA_X+32), Y(NOAGA_Y));
  SpriteUpdatePos(&noaga[3], X(NOAGA_X+48), Y(NOAGA_Y));

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  LoadColors(loader_colors, 0);
  LoadColors(ufo_colors, 16);
  LoadColors(ufo_colors, 20);
  ITER(i, 24, 31, SetColor(i, 0xf90));


  EnableDMA(DMAF_BLITTER);
  BitmapCopy(screen, 0, 0, &loader);
  WaitBlitter();
  DisableDMA(DMAF_BLITTER);

  CopSetupBitplanes(cp, screen, DEPTH);
  CopListFinish(cp);
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER | DMAF_SPRITE);

}

static void Kill(void) {
  CopperStop();
  DeleteCopList(cp);

  DeleteBitmap(screen);

  PtEnd();
  PtRemoveCIA();
  DisableDMA(DMAF_AUDIO);
}

static void MoveUfo(void) {
  static short counter = 0;
  static short ufo_y = 16;
  static short dir = 1;
  static short s = 0;
  short i = 0;

  if (ufo_y <= 16) {
    dir = 1;
  } else if (ufo_y >= 22) {
    dir = -1;
  }

  ++counter;
  if (counter % 7 == 0) {
    ufo_y += dir;
  }

  SpriteUpdatePos(&ufo[0], X(16), Y(ufo_y));
  SpriteUpdatePos(&ufo[1], X(32), Y(ufo_y));
  SpriteUpdatePos(&ufo[2], X(48), Y(ufo_y));

  if (show_noaga && s < 16) {
    for (i = 0; i < 4; ++i) {
      SetColor(24+i, ColorTransition(0xf90, noaga_colors[i], s));
      SetColor(28+i, ColorTransition(0xf90, noaga_colors[i], s));
    }
    ++s;
  }

}

static void Render(void) {
  static __code short x = 0;
  short newX = frameCount >> 3;
  if (newX > 24) {
    show_noaga = true;
  }
  if (newX > 121)
    newX = 122;
  for (; x < newX; x++) {
    CpuLine(X1 + x, Y1, X1 + x, Y2);
  }
}

EFFECT(Loader, NULL, NULL, Init, Kill, Render, MoveUfo);
