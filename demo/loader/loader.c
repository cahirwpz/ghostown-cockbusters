#include <effect.h>
#include <copper.h>
#include <blitter.h>
#include <gfx.h>
#include <line.h>

#include "data/loader.c"

static __code BitmapT *screen;
static __code CopListT *cp;

#define X1 96
#define Y1 (120 + 32)
#define X2 224
#define Y2 (136 + 24)

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 1

static void Init(void) {
  screen = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  cp = NewCopList(40);

  CpuLineSetup(screen, 0);
  CpuLine(X1 - 1, Y1 - 2, X2 + 1, Y1 - 2);
  CpuLine(X1 - 1, Y2 + 1, X2 + 1, Y2 + 1);
  CpuLine(X1 - 2, Y1 - 1, X1 - 2, Y2 + 1);
  CpuLine(X2 + 2, Y1 - 1, X2 + 2, Y2 + 1);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  LoadColors(loader_colors, 0);

  EnableDMA(DMAF_BLITTER);
  BitmapCopy(screen, (WIDTH - loader_width) / 2, Y1 - loader_height - 16,
             &loader);
  WaitBlitter();
  DisableDMA(DMAF_BLITTER);

  CopSetupBitplanes(cp, screen, DEPTH);
  CopListFinish(cp);
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  CopperStop();
  DeleteCopList(cp);

  DeleteBitmap(screen);
}

static void Render(void) {
  static __code short x = 0;
  short newX = frameCount >> 1;
  if (newX > 128)
    newX = 129;
  for (; x < newX; x++) {
    CpuLine(X1 + x, Y1, X1 + x, Y2);
  }
}

EFFECT(Loader, NULL, NULL, Init, Kill, Render, NULL);
