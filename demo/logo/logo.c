#include <effect.h>
#include <copper.h>
#include <blitter.h>
#include <gfx.h>

#include "data/logo.c"

static __code CopListT *cp;

/*
 * http://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node0281.html
 */

typedef struct ColorCycling {
  short rate;
  short step;
  short len;
  short indices[16];
  short colors[16];
} ColorCyclingT;

static ColorCyclingT logo_cycling[] = {
  {
    .rate = 5632,
    .step = 0,
    .len = 8,
    .indices = {26, 5, 7, 27, 27, 7, 5, 26},
    .colors = {}
  }, {
    .rate = 5632,
    .step = 0,
    .len = 4,
    .indices = {23, 24, 25, 23},
    .colors = {}
  }, {
    .rate = 4608,
    .step = 0,
    .len = 8,
    .indices = {10, 10, 29, 28, 10, 29, 10, 10},
    .colors = {}
  }, {
    .rate = 3072,
    .step = 0,
    .len = 11,
    .indices = {18, 22, 30, 31, 31, 31, 30, 22, 18, 18, 18},
    .colors = {}
  }
};

/* I genuinely hate this format. Reverse engineering it is PITA! */
static void ColorCyclingInit(ColorCyclingT *rots, short len, u_short *colors) {
  short i, j;

  for (j = 0; j < len; j++) {
    ColorCyclingT *rot = &rots[j];
    rot->step = 0;
    rot->rate = div16(1 << 24, rot->rate);
 
    for (i = 0; i < rot->len; i++)
      rot->colors[i] = colors[rot->indices[i]];
  }
}

#define FRAME ((1 << 24) / (1 << 14))

static void ColorCyclingStep(ColorCyclingT *rots, short len) {
  // From https://wiki.amigaos.net/wiki/ILBM_IFF_Interleaved_Bitmap#CRNG
  // "The field rate determines the speed at which the colors will step when
  // color cycling is on. The units are such that a rate of 60 steps per
  // second is represented as 2^14 = 16384. Slower rates can be obtained
  // by linear scaling: for 30 steps/second, rate = 8192; for 1 step/second,
  // rate = 16384/60 ~273."
  short j;

  for (j = 0; j < len; j++) {
    ColorCyclingT *rot = &rots[j];
    short i;
    u_short tmp;

    rot->step += FRAME;
    if (rot->step < FRAME * 2)
      continue;
    rot->step -= FRAME * 2;

    tmp = rot->colors[0];
    for (i = 0; i < rot->len - 1; i++) {
      rot->colors[i] = rot->colors[i + 1];
    }
    rot->colors[i] = tmp;

    for (i = rot->len - 1; i >= 0; i--) {
      SetColor(rot->indices[i], rot->colors[i]);
    }

  }
}

static void Init(void) {
  SetupPlayfield(MODE_LORES, logo_depth, X(0), Y(0), logo_width, logo_height);
  LoadColors(logo_colors, 0);

  ColorCyclingInit(logo_cycling, nitems(logo_cycling), logo_colors);

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

static void Render(void) {
  ColorCyclingStep(logo_cycling, nitems(logo_cycling));
  TaskWaitVBlank();
}

EFFECT(Logo, NULL, NULL, Init, Kill, Render, NULL);
