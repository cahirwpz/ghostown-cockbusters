#include <effect.h>
#include <copper.h>
#include <blitter.h>
#include <gfx.h>
#include <sync.h>

#include "data/logo.c"
#include "data/jitter.c"
#include "palette.h"

static __code CopListT *cp;

/*
 * http://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node0281.html
 */

typedef struct ColorCycling {
  short rate;
  short step;
  short ncolors;
  short *cells;
  u_short *colors;
} ColorCyclingT;

#include "data/logo-cycling.c"

/* I genuinely hate this format. Reverse engineering it is PITA! */
#define PAL_FRAME ((1 << 16) / 50)

static void ColorCyclingStep(ColorCyclingT *rots, short len) {
  do {
    ColorCyclingT *rot = rots++;

    rot->step += PAL_FRAME;

    if (rot->step >= rot->rate) {
      short *cells = rot->cells;
      short reg;

      rot->step -= rot->rate;

      while ((reg = *cells++) >= 0) {
        short idx = *cells + 1; 
        if (idx == rot->ncolors)
          idx = 0;
        *cells++ = idx;

        SetColor(reg, rot->colors[idx]);
      }
    }
  } while (--len > 0);
}

static void Load(void) {
  TrackInit(&JitterLogo);
}

static void Init(void) {
  TimeWarp(jitter_logo_start);

  SetupPlayfield(MODE_LORES, logo_depth, X(0), Y(0), logo_width, logo_height);
  LoadColors(logo_colors, 0);

  cp = NewCopList(40);
  CopSetupBitplanes(cp, &logo, logo_depth);
  CopListFinish(cp);
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  CopperStop();
  DeleteCopList(cp);
}

static __code short jitter[14] = {
  0, 0,
  -1, 1,
  -2, 2,
  -3, 3,
  -4, 4,
  -5, 5,
  -6, 6,
};

static void Render(void) {
  static bool enable = false;
  short val = TrackValueGet(&JitterLogo, frameCount);

  if (enable) {
    LoadColors(logo_colors, 0);
    ColorCyclingStep(logo_cycling, nitems(logo_cycling));
  }

  if (val > 0) {
    if (!enable) {
      short i;
      enable = true;
      for (i = 0; i < 32; i++) {
        SetColor(i, 0xfff);
      }
    }
    SetupDisplayWindow(MODE_LORES, X(0), Y(jitter[val]), logo_width, logo_height);
  }

  TaskWaitVBlank();
}

EFFECT(Logo, Load, NULL, Init, Kill, Render, NULL);
