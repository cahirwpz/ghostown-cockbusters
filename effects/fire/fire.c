#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <pixmap.h>
#include <system/interrupt.h>
#include <system/memory.h>
#include <stdlib.h>
#include <common.h>

#define WIDTH 80
#define HEIGHT 64
#define DEPTH 4


static uint32_t dualtab[256] = {
  0, 0, 0, 0, 0, 262147, 262147, 262147, 262147, 524336, 524336, 524336, 524336, 786483, 786483, 786483, 786483, 1049344, 1049344, 1049344, 1049344, 1311495, 1311495, 1311495, 1311495, 1573684, 1573684, 1573684, 1573684, 1835891, 1835891, 1835891, 1835891, 2109504, 2109504, 2109504, 2109504, 2371652, 2371652, 2371652, 2371652, 2633799, 2633799, 2633799, 2633799, 2896944, 2896944, 2896944, 2896944, 3159091, 3159091, 3159091, 3159091, 3421239, 3421239, 3421239, 3421239, 3684100, 3684100, 3684100, 3684100, 3946304, 3946304, 3946304, 3946304, 3946304, 4208452, 4208452, 4208452, 4208452, 4470599, 4470599, 4470599, 4470599, 4748080, 4748080, 4748080, 4748080, 5010228, 5010228, 5010228, 5010228, 5272375, 5272375, 5272375, 5272375, 5534519, 5534519, 5534519, 5534519, 5796723, 5796723, 5796723, 5796723, 6058867, 6058867, 6058867, 6058867, 6321015, 6321015, 6321015, 6321015, 6583159, 6583159, 6583159, 6583159, 6846259, 6846259, 6846259, 6846259, 7108403, 7108403, 7108403, 7108403, 7370551, 7370551, 7370551, 7370551, 7632695, 7632695, 7632695, 7632695, 7894899, 7894899, 7894899, 7894899, 8157043, 8157043, 8157043, 8157043, 8157043, 8419191, 8419191, 8419191, 8419191, 8681335, 8681335, 8681335, 8681335, 8943476, 8943476, 8943476, 8943476, 9205620, 9205620, 9205620, 9205620, 9467719, 9467719, 9467719, 9467719, 9729863, 9729863, 9729863, 9729863, 9992004, 9992004, 9992004, 9992004, 10254148, 10254148, 10254148, 10254148, 10515575, 10515575, 10515575, 10515575, 10777719, 10777719, 10777719, 10777719, 11039860, 11039860, 11039860, 11039860, 11302004, 11302004, 11302004, 11302004, 11564103, 11564103, 11564103, 11564103, 11826247, 11826247, 11826247, 11826247, 12088388, 12088388, 12088388, 12088388, 12350532, 12350532, 12350532, 12350532, 12350532, 12601207, 12601207, 12601207, 12601207, 12863351, 12863351, 12863351, 12863351, 13125492, 13125492, 13125492, 13125492, 13387636, 13387636, 13387636, 13387636, 13649735, 13649735, 13649735, 13649735, 13911879, 13911879, 13911879, 13911879, 14174020, 14174020, 14174020, 14174020, 14436164, 14436164, 14436164, 14436164, 14697591, 14697591, 14697591, 14697591, 14959735, 14959735, 14959735, 14959735, 15221876, 15221876, 15221876, 15221876, 15484020, 15484020, 15484020, 15484020, 15746119, 15746119, 15746119, 15746119, 16008263, 16008263, 16008263, 16008263, 16270404, 16270404, 16270404, 16270404
};
/*
 * Precalculed values for int(sum * 63 / 256)
 * Affects fire size
 */
static short lookupTable[256] = {
  0, 0, 0, 0, 0,
  1, 1, 1, 1,
  2, 2, 2, 2,
  3, 3, 3, 3,
  4, 4, 4, 4,
  5, 5, 5, 5,
  6, 6, 6, 6,
  7, 7, 7, 7,
  8, 8, 8, 8,
  9, 9, 9, 9,
  10, 10, 10, 10,
  11, 11, 11, 11,
  12, 12, 12, 12,
  13, 13, 13, 13,
  14, 14, 14, 14,
  15, 15, 15, 15, 15,
  16, 16, 16, 16,
  17, 17, 17, 17,
  18, 18, 18, 18,
  19, 19, 19, 19,
  20, 20, 20, 20,
  21, 21, 21, 21,
  22, 22, 22, 22,
  23, 23, 23, 23,
  24, 24, 24, 24,
  25, 25, 25, 25,
  26, 26, 26, 26,
  27, 27, 27, 27,
  28, 28, 28, 28,
  29, 29, 29, 29,
  30, 30, 30, 30,
  31, 31, 31, 31, 31,
  32, 32, 32, 32,
  33, 33, 33, 33,
  34, 34, 34, 34,
  35, 35, 35, 35,
  36, 36, 36, 36,
  37, 37, 37, 37,
  38, 38, 38, 38,
  39, 39, 39, 39,
  40, 40, 40, 40,
  41, 41, 41, 41,
  42, 42, 42, 42,
  43, 43, 43, 43,
  44, 44, 44, 44,
  45, 45, 45, 45,
  46, 46, 46, 46,
  47, 47, 47, 47, 47,
  48, 48, 48, 48,
  49, 49, 49, 49,
  50, 50, 50, 50,
  51, 51, 51, 51,
  52, 52, 52, 52,
  53, 53, 53, 53,
  54, 54, 54, 54,
  55, 55, 55, 55,
  56, 56, 56, 56,
  57, 57, 57, 57,
  58, 58, 58, 58,
  59, 59, 59, 59,
  60, 60, 60, 60,
  61, 61, 61, 61,
  62, 62, 62, 62,
};
/*
 * Palette was extented from 42 to 64 colors simply by duplicating some values.
 * May need adjustment for better result.
 * Colors in those arrays are coded in c2p format
 * [r0 g0 b0 b0 r1 g1 b1 b1 r2 g2 b2 b2 r3 g3 b3 b3]
 */
static short paletteRed[64] = {
  0x0000,
  0x0008,
  0x0080,
  0x0088,
  0x0800,
  0x080c,
  0x0884,
  0x08c8,
  0x8040,
  0x8044,
  0x804c,
  0x8480,
  0x8488,
  0x848c,
  0x8c04,
  0x8c40,
  0x8c44,
  0x8c4c,
  0xc880,
  0xc884,
  0xc88c, 0xc88c,
  0xc8c8, 0xc8c8,
  0xc8cc, 0xc8cc,
  0xcc88, 0xcc88,
  0xcc8c, 0xcc8c,
  0xccc8, 0xccc8,
  0xcccc, 0xcccc,
  0xccc4, 0xccc4,
  0xcc4c, 0xcc4c,
  0xcc44, 0xcc44,
  0xc4cc, 0xc4cc,
  0xc4c4, 0xc4c4,
  0xc44c, 0xc44c,
  0xc444, 0xc444,
  0x4ccc, 0x4ccc,
  0x4cc4, 0x4cc4,
  0x4c4c, 0x4c4c,
  0x4c44, 0x4c44,
  0x44cc, 0x44cc,
  0x44c4, 0x44c4,
  0x444c, 0x444c,
  0x4444, 0x4444,
};
static short paletteBlue[64] = {
  0x0000,
  0x0003,
  0x0030,
  0x0033,
  0x0300,
  0x0307,
  0x0334,
  0x0373,
  0x3040,
  0x3044,
  0x3047,
  0x3430,
  0x3433,
  0x3437,
  0x3704,
  0x3740,
  0x3744,
  0x3747,
  0x7330,
  0x7334,
  0x7337, 0x7337,
  0x7373, 0x7373,
  0x7377, 0x7377,
  0x7733, 0x7733,
  0x7737, 0x7737,
  0x7773, 0x7773,
  0x7777, 0x7777,
  0x7774, 0x7774,
  0x7747, 0x7747,
  0x7744, 0x7744,
  0x7477, 0x7477,
  0x7474, 0x7474,
  0x7447, 0x7447,
  0x7444, 0x7444,
  0x4777, 0x4777,
  0x4774, 0x4774,
  0x4747, 0x4747,
  0x4744, 0x4744,
  0x4477, 0x4477,
  0x4474, 0x4474,
  0x4447, 0x4447,
  0x4444, 0x4444,
};

static short *palette = paletteBlue;
static short *chunky;
static short *buf;
static BitmapT *screen;
static CopListT *cp;
static CopInsPairT *bplptr;


static struct {
  short phase;
  void **bpl;
  void *chunky;
} c2p = { 256, NULL, NULL };

#define BPLSIZE ((WIDTH * 4) * HEIGHT / 8) /* 2560 bytes */
#define BLTSIZE ((WIDTH * 4) * HEIGHT / 2) /* 10240 bytes */

static void ChunkyToPlanar(void) {
  void *src = c2p.chunky;
  void *dst = c2p.chunky + BLTSIZE;
  void **bpl = c2p.bpl;

  switch (c2p.phase) {
    case 0:
      /* Initialize chunky to planar. */
      custom->bltamod = 4;
      custom->bltbmod = 4;
      custom->bltdmod = 4;
      custom->bltcdat = 0x00FF;
      custom->bltafwm = -1;
      custom->bltalwm = -1;

      /* Swap 8x4, pass 1. */
      custom->bltapt = src + 4;
      custom->bltbpt = src;
      custom->bltdpt = dst;

      /* ((a >> 8) & 0x00FF) | (b & ~0x00FF) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ANBC | ABNC | NABNC) | ASHIFT(8);
      custom->bltcon1 = 0;
      custom->bltsize = 2 | ((BLTSIZE / 16) << 6);
      break;

    case 1:
      custom->bltsize = 2 | ((BLTSIZE / 16) << 6);
      break;

    case 2:
      /* Swap 8x4, pass 2. */
      custom->bltapt = src + BLTSIZE - 6;
      custom->bltbpt = src + BLTSIZE - 2;
      custom->bltdpt = dst + BLTSIZE - 2;

      /* ((a << 8) & ~0x00FF) | (b & 0x00FF) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | (ABNC | ANBNC | ABC | NABC) | ASHIFT(8);
      custom->bltcon1 = BLITREVERSE;
      custom->bltsize = 2 | ((BLTSIZE / 16) << 6);
      break;

    case 3:
      custom->bltsize = 2 | ((BLTSIZE / 16) << 6);
      break;

    case 4:
      custom->bltamod = 6;
      custom->bltbmod = 6;
      custom->bltdmod = 0;
      custom->bltcdat = 0x0F0F;

      custom->bltapt = dst + 2;
      custom->bltbpt = dst;
      custom->bltdpt = bpl[0];

      /* ((a >> 4) & 0x0F0F) | (b & ~0x0F0F) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ANBC | ABNC | NABNC) | ASHIFT(4);
      custom->bltcon1 = 0;
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 5:
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 6:
      custom->bltapt = dst + 6;
      custom->bltbpt = dst + 4;
      custom->bltdpt = bpl[2];
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 7:
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 8:
      custom->bltapt = dst + BLTSIZE - 8;
      custom->bltbpt = dst + BLTSIZE - 6;
      custom->bltdpt = bpl[1] + BPLSIZE - 2;

      /* ((a << 8) & ~0x0F0F) | (b & 0x0F0F) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | (ABNC | ANBNC | ABC | NABC) | ASHIFT(4);
      custom->bltcon1 = BLITREVERSE;
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 9:
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 10:
      custom->bltapt = dst + BLTSIZE - 4;
      custom->bltbpt = dst + BLTSIZE - 2;
      custom->bltdpt = bpl[3] + BPLSIZE - 2;
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 11:
      custom->bltsize = 1 | ((BLTSIZE / 16) << 6);
      break;

    case 12:
      CopInsSet32(&bplptr[0], bpl[3]);
      CopInsSet32(&bplptr[1], bpl[2]);
      CopInsSet32(&bplptr[2], bpl[1]);
      CopInsSet32(&bplptr[3], bpl[0]);
      break;

    default:
      break;
  }

  c2p.phase++;

  ClearIRQ(INTF_BLIT);
}

static CopListT *MakeCopperList(void) {
  CopListT *cp = NewCopList(1200);
  short i;

  bplptr = CopSetupBitplanes(cp, screen, DEPTH);
  CopLoadColor(cp, 0, 15, 0);
  for (i = 0; i < HEIGHT * 4; i++) {
    CopWaitSafe(cp, Y(i), 0);
    /* Line quadrupling. */
    CopMove16(cp, bpl1mod, ((i & 3) != 3) ? -40 : 0);
    CopMove16(cp, bpl2mod, ((i & 3) != 3) ? -40 : 0);
    /* Alternating shift by one for bitplane data. */
    CopMove16(cp, bplcon1, (i & 1) ? 0x0022 : 0x0000);
  }
  return CopListFinish(cp);
}

static void ShowColors(void) {
  short i;

  for (i = 0; i < WIDTH * 64; ++i) {
    chunky[i] = palette[i/WIDTH];
  }
}

static void Init(void) {
  /* To avoid 'defined but not used' error */
  (void)dualtab;
  (void)lookupTable;
  (void)paletteRed;
  (void)paletteBlue;
  (void)ShowColors;
  screen = NewBitmap(WIDTH * 4, HEIGHT, DEPTH, BM_CLEAR);

  chunky = MemAlloc((WIDTH * 4) * HEIGHT, MEMF_CHIP);

  buf = MemAlloc(WIDTH * HEIGHT * 2, MEMF_CHIP);

  EnableDMA(DMAF_BLITTER);

  BitmapClear(screen);

  SetupPlayfield(MODE_HAM, 7, X(0), Y(0), WIDTH * 4 + 2, HEIGHT * 4);

  custom->bpldat[4] = 0x7777; // rgbb: 0111
  custom->bpldat[5] = 0xcccc; // rgbb: 1100

  cp = MakeCopperList();
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER);

  SetIntVector(INTB_BLIT, (IntHandlerT)ChunkyToPlanar, NULL);
  EnableINT(INTF_BLIT);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER);

  DisableINT(INTF_BLIT);
  ResetIntVector(INTB_BLIT);

  DeleteCopList(cp);

  MemFree(chunky);

  MemFree(buf);

  DeleteBitmap(screen);
}

static void RandomizeBottom(void) {
  int r;
  short i;
  short *bufPtr;

  /*
   * Bottom two lines are randomized every frame and not displayed.
   * This loop takes 51 raster lines (on average) to execute.
   */
  bufPtr = &(buf[WIDTH*HEIGHT - 1]);
  for (i = 1; i <= WIDTH*2; i+=5) {
    r = random();
    *bufPtr-- = (r & 0x3F) * 4;
    r >>= 6;
    *bufPtr-- = (r & 0x3F) * 4;
    r >>= 6; 
    *bufPtr-- = (r & 0x3F) * 4;
    r >>= 6; 
    *bufPtr-- = (r & 0x3F)  * 4;
    r >>= 6; 
    *bufPtr-- = (r & 0x3F) * 4;
  }
}

static void MainLoop(void) {
  /*
   * Right now this effect takes 1141-1320-1328 (min-avg-max) raster lines to render.
   */
  uint32_t aux;
  short i, v;
  short *bufPtr;
  short *chunkyPtr;

  bufPtr = buf;
  chunkyPtr = chunky;
  for (i = 0; i < WIDTH * HEIGHT - 2*WIDTH; ++i) {
    v  = bufPtr[WIDTH];
    v += bufPtr[WIDTH-1];
    v += bufPtr[WIDTH+1];
    v += bufPtr[WIDTH*2];

    asm("movel (%2,%1:w),%0"
      : "=r" (aux)
      : "d" (v), "a" (dualtab));

    *chunkyPtr++ = aux;

    asm("swap %0"
        : "+d" (aux));
    asm("movew %1,(%0)+"
        : "=r" (bufPtr), "+d" (aux));
  }
}

PROFILE(Fire);

static void Render(void) {
  (void)ShowColors;
  ProfilerStart(Fire);
  {
    RandomizeBottom();
    MainLoop();
  }
  ProfilerStop(Fire);

  c2p.phase = 0;
  c2p.chunky = chunky;
  c2p.bpl = screen->planes;
  ChunkyToPlanar();
}

EFFECT(Fire, NULL, NULL, Init, Kill, Render, NULL);
