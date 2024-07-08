#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <sprite.h>
#include <system/memory.h>
#include <system/interrupt.h>
#include <common.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 2


static CopListT *cp;
static CopInsPairT *bplptr;
static BitmapT *screen[2];
static short active = 0;


static void BlitHorizontal0(void **planes, short pos, short thickness) {
  if (pos > 256) {
    return;
  }
  if (pos + thickness >= 256) {
    thickness = 256 - pos;
  }
  if (pos < 0) {
    thickness += pos;
    pos = 0;
  }
  if (thickness < 1) {
    return;
  }

  custom->bltamod = 0;
  custom->bltbmod = 0;
  custom->bltcmod = 0;
  custom->bltdmod = 0;

  custom->bltafwm = -1;
  custom->bltalwm = -1;

  custom->bltdpt = planes[0] + pos*40;

  custom->bltcon0 = DEST | 0xFF;
  custom->bltcon1 = 0;
  custom->bltsize = (thickness << 6) | 20;

  WaitBlitter();
}

static void BlitHorizontal1(void **planes, short pos, short thickness) {
  if (pos > 256) {
    return;
  }
  if (pos + thickness >= 256) {
    thickness = 256 - pos;
  }
  if (pos < 0) {
    thickness += pos;
    pos = 0;
  }
  if (thickness < 1) {
    return;
  }

  custom->bltamod = 0;
  custom->bltbmod = 0;
  custom->bltcmod = 0;
  custom->bltdmod = 0;

  custom->bltafwm = -1;
  custom->bltalwm = -1;

  custom->bltdpt = planes[1] + pos*40;

  custom->bltcon0 = DEST | 0xFF;
  custom->bltcon1 = 0;
  custom->bltsize = (thickness << 6) | 20;

  WaitBlitter();
}

static void BlitHorizontal(void **planes, short pos, short thickness) {
  /* thickness = 1 or 3 or 5 */
  BlitHorizontal0(planes, pos, thickness);
  BlitHorizontal1(planes, pos+1, thickness-2);
}

static void BlitVertical(void **planes, short pos, short thickness) {
  short i = 0;

  for (i = 0; i < thickness; i++) {
    if (pos >= 320) {
      break;
    }
    custom->bltamod = 0;
    custom->bltbmod = 40-1; //thickness;
    custom->bltcmod = 0;
    custom->bltdmod = 40-1; //thickness;

    custom->bltadat = 0x8000 >> (pos%16);

    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltbpt = planes[0] + pos/8;
    custom->bltdpt = planes[0] + pos/8;

    custom->bltcon0 = (DEST | SRCB) | (A_OR_B);
    custom->bltcon1 = 0;
    custom->bltsize = (256 << 6) | 1;

    ++pos;
    WaitBlitter();
  }
}

static void ClearBitplanes(void **planes) {
  custom->bltdmod = 0;
  custom->bltcon0 = (DEST) | 0x0;
  custom->bltcon1 = 0;

  custom->bltdpt = planes[0];
  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();

  custom->bltdmod = 0;
  custom->bltcon0 = (DEST) | 0x0;
  custom->bltcon1 = 0;

  custom->bltdpt = planes[1];
  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();
}


static CopListT *MakeCopperList(void) {
  cp = NewCopList(128);
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);

  CopListFinish(cp);

  return cp;
}

static void Init(void) {
  EnableDMA(DMAF_BLITTER | DMAF_RASTER);

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, 256);

  SetColor(0, 0x000);
  SetColor(1, 0x822);
  SetColor(2, 0xFF0);
  SetColor(3, 0xFAA);

  cp = MakeCopperList();
  CopListActivate(cp);
}

static void Kill(void) {
  DisableDMA(DMAF_BLITTER | DMAF_RASTER);

  DeleteCopList(cp);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

PROFILE(Darkroom);

static void Render(void) {
  static short posx = 160;
  static short posy = 128;
  (void)ClearBitplanes;
  (void)BlitHorizontal;
  (void)BlitVertical;
  (void)posx;
  (void)posy;
  ProfilerStart(Darkroom);
  {
    ClearBitplanes(screen[active]->planes);

    BlitHorizontal(screen[active]->planes, posy, 3);
    // posy += 1;
    if (posy >= 256) {
      posy = -20;
    }

    // BlitVertical(screen[active]->planes, posx, 5);
    // posx += 1;
    // if (posx >= 320) {
    //   posx = 0;
    // }

    ITER(i, 0, DEPTH - 1, CopInsSet32(&bplptr[i], screen[active]->planes[i]));
  }
  ProfilerStop(Darkroom);

  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Darkroom, NULL, NULL, Init, Kill, Render, NULL);