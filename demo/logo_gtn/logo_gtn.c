#include <effect.h>
#include <copper.h>
#include <blitter.h>
#include <gfx.h>

#include "data/logo_gtn.c"

static CopListT *cp;

static void Init(void) {
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

static void Render(void) {
  TaskWaitVBlank();
}

EFFECT(LogoGtn, NULL, NULL, Init, Kill, Render, NULL);
