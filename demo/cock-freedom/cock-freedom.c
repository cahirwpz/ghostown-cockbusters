#include <2d.h>
#include <blitter.h>
#include <copper.h>
#include <effect.h>
#include <fx.h>
#include <line.h>
#include <pixmap.h>
#include <types.h>
#include <sync.h>
#include <system/memory.h>

#define WIDTH 320
#define HEIGHT 180
#define YOFF ((256 - HEIGHT) / 2)
#define DEPTH 4

static __code BitmapT *screen;
static __code CopInsPairT *bplptr;
static __code CopListT *cp;
static __code short active = 0;
static __code short maybeSkipFrame = 0;
static __code short oldgradientno = 0;

#include "data/cock_scene_4.c"
#include "data/cock-pal.c"
#include "data/cock-pal2.c"
#include "data/cock-pal3.c"
#include "data/cock-pal4.c"
#include "data/cock-pal5.c"
#include "data/cock-pal6.c"
#include "data/cock-pal7.c"
#include "data/cock-pal8.c"
#include "data/cock-pal9.c"
#include "data/cock-pal10.c"
#include "data/cock-pal11.c"

#include "data/cock-freedom.c"

static const PixmapT *palettes[] = {
  &gradient, &gradient2, &gradient3,
  &gradient4,  &gradient5,  &gradient6,
  &gradient7,  &gradient8,  &gradient9,
  &gradient10,  &gradient11
};

/* Reading polygon data */
static __code short current_frame = 0;

static CopListT *MakeCopperList(CopListT *cp, short gno) {
  bplptr = CopSetupBitplanes(cp, screen, DEPTH);
  {
    short *pixels = palettes[gno]->pixels;
    short i, j;

    for (i = 0; i < HEIGHT / 10; i++) {
      u_short c;
      u_short k, n;

      CopWait(cp, Y(YOFF + i * 10 - 1), 0xde);

      CopSetColor(cp, 0, 0);
      for (j = 1, n = 1; j < 16; n += n) {
        c = *pixels++;
        for (k = 0; k < n; k++, j++)
          CopSetColor(cp, j, c);
      }
    }
  }
  return CopListFinish(cp);
}

static void Init(void) {
  TimeWarp(cock_freedom_start);
  TrackInit(&freedom_gradientno);

  screen = NewBitmap(WIDTH, HEIGHT, DEPTH + 1, BM_CLEAR);
  EnableDMA(DMAF_BLITTER);
  BitmapClear(screen);
  WaitBlitter();

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(YOFF), WIDTH, HEIGHT);
  cp = NewCopList(100 + gradient_height * ((1 << DEPTH) + 1));
  MakeCopperList(cp, 0);

  CopListActivate(cp);
  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  BlitterStop();
  CopperStop();
  DeleteCopList(cp);
  DeleteBitmap(screen);
}

static inline void DrawEdge(short *coords, void *dst,
                            CustomPtrT custom_ asm("a6")) {
  /* XXX awful hack to find some unused chip memory to write first pixel to!
   * This avoids creating small memory allocation. */
  short *tmp = (short *)cp->curr;

  short bltcon0, bltcon1, bltsize, bltbmod, bltamod;
  short dmin, dmax, derr, offset;
  int bltapt;

  short x0 = *coords++;
  short y0 = *coords++;
  short x1 = *coords++;
  short y1 = *coords++;

  if (y0 == y1)
    return;

  if (y0 > y1) {
    swapr(x0, x1);
    swapr(y0, y1);
  }

  dmax = x1 - x0;
  if (dmax < 0)
    dmax = -dmax;

  dmin = y1 - y0;
  if (dmax >= dmin) {
    if (x0 >= x1)
      bltcon1 = AUL | SUD | LINEMODE | ONEDOT;
    else
      bltcon1 = SUD | LINEMODE | ONEDOT;
  } else {
    if (x0 >= x1)
      bltcon1 = SUL | LINEMODE | ONEDOT;
    else
      bltcon1 = LINEMODE | ONEDOT;
    swapr(dmax, dmin);
  }

  offset = ((y0 << 5) + (y0 << 3) + (x0 >> 3)) & ~1;

  bltcon0 = rorw(x0 & 15, 4) | BC0F_LINE_EOR;
  bltcon1 |= rorw(x0 & 15, 4);

  dmin <<= 1;
  derr = dmin - dmax;
  if (derr < 0)
    bltcon1 |= SIGNFLAG;

  bltamod = derr - dmax;
  bltbmod = dmin;
  bltsize = (dmax << 6) + 66;
  bltapt = derr;

  _WaitBlitter(custom_);

  custom_->bltcon0 = bltcon0;
  custom_->bltcon1 = bltcon1;
  custom_->bltapt = (void *)bltapt;
  custom_->bltbmod = bltbmod;
  custom_->bltamod = bltamod;
  custom_->bltcpt = dst + offset;
  custom_->bltdpt = tmp;
  custom_->bltsize = bltsize;
}

/* Get (x,y) on screen position from linear memory repr */
#define CalculateXY(data, x, y)         \
  asm("clrl  %0\n\t"                    \
      "movew %2@+,%0\n\t"               \
      "divu  %3,%0\n\t"                 \
      "movew %0,%1\n\t"                 \
      "swap  %0"                        \
      : "=d" (x), "=r" (y), "+a" (data) \
      : "i" (WIDTH));

static short *PickFrame(void) {
  short num = current_frame++;

  for (;;) {
    if (num < cock_scene_4_frames)
      return cock_scene_4_frame[num];
    num -= cock_scene_4_frames;
  }
}

static void DrawFrame(void *dst, CustomPtrT custom_ asm("a6")) {
  static __code Point2D points[256];
  short *data = PickFrame();
  short n;

  _WaitBlitter(custom_);

  /* prepare for line drawing */
  custom_->bltafwm = 0xffff;
  custom_->bltalwm = 0xffff;
  custom_->bltbdat = 0xffff;
  custom_->bltadat = 0x8000;
  custom_->bltcmod = WIDTH / 8;
  custom_->bltdmod = WIDTH / 8;

  while ((n = *data++)) {
    {
      short *point = (short *)points;
      short k = n;
      while (--k >= 0) {
        short x = 0, y = 0;
        CalculateXY(data, x, y);
        *point++ = x;
        *point++ = y;
      }
      *point++ = points[0].x;
      *point++ = points[0].y;
    }

    {
      Point2D *point = points;
      while (--n >= 0)
        DrawEdge((short *)point++, dst, custom_);
    }
  }
}

PROFILE(AnimRender);

static void Render(void) {
  /* Frame lock the effect to 25 FPS */
  if (maybeSkipFrame) {
    maybeSkipFrame = 0;
    if (frameCount - lastFrameCount == 1) {
      TaskWaitVBlank();
      return;
    }
  }

  ProfilerStart(AnimRender);
  {
    BlitterClear(screen, active);
    DrawFrame(screen->planes[active], custom);
    BlitterFill(screen, active);
  }
  ProfilerStop(AnimRender);

  {
    short n = DEPTH;

    while (--n >= 0) {
      short i = mod16(active + n + 1 - DEPTH, DEPTH + 1);
      if (i < 0) i += DEPTH + 1;
      CopInsSet32(&bplptr[n], screen->planes[i]);
    }
  }

  {
    u_short gno = TrackValueGet(&freedom_gradientno, frameCount);
    if (oldgradientno != gno) {
      oldgradientno = gno;
      // We just need to regenerate the list starting w/ the first instruction
      CopListReset(cp);
      MakeCopperList(cp, gno);
    }
  }

  CopListRun(cp);
  TaskWaitVBlank();
  active = mod16(active + 1, DEPTH + 1);
  maybeSkipFrame = 1;
}

EFFECT(CockFreedom, NULL, NULL, Init, Kill, Render, NULL);
