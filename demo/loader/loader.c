#include <effect.h>
#include <copper.h>
#include <blitter.h>
#include <gfx.h>
#include <line.h>
#include <sprite.h>

#include "data/loader.c"
#include "data/ufo.c"

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

static void Init(void) {
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

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  LoadColors(loader_colors, 0);
  LoadColors(ufo_colors, 16);
  LoadColors(ufo_colors, 20);

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
}

static void MoveUfo(void) {
  static short counter = 0;
  static short ufo_y = 16;
  static short dir = 1;

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

}

static void Render(void) {
  static __code short x = 0;
  short newX = frameCount >> 3;
  if (newX > 121)
    newX = 122;
  for (; x < newX; x++) {
    CpuLine(X1 + x, Y1, X1 + x, Y2);
  }
}

EFFECT(Loader, NULL, NULL, Init, Kill, Render, MoveUfo);
