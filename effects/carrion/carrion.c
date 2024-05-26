#include "effect.h"
#include "copper.h"
#include "pixmap.h"
#include "gfx.h"

#include "data/carrion-conan-pal.c"
#include "data/carrion-conan-data.c"

static CopListT *cp;

static CopListT *MakeCopperList(void) {
  CopListT *cp = NewCopList(100 + conan_height * (conan_cols_width + 1));

  CopSetupBitplanes(cp, &conan, conan_depth);

  {
    u_short *data = conan_cols_pixels;
    short i, j;

    for (i = 0; i < conan_height; i++) {
      for (j = 0; j < conan_cols_width; j++) {
        CopMove16(cp, color[j], *data++);
      }

      /* Start exchanging palette colors at the end of line. */
      CopWait(cp, Y(i-1), X(conan_width + 16));
    }
  }

  return CopListFinish(cp);
}

static void Init(void) {
  SetupPlayfield(MODE_LORES, conan_depth, X(0), Y(0), conan_width, conan_height);

  cp = MakeCopperList();
  CopListActivate(cp);
  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  DeleteCopList(cp);
}

EFFECT(Carrion, NULL, NULL, Init, Kill, NULL, NULL);
