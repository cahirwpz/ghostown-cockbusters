#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <fx.h>
#include <pixmap.h>
#include <system/interrupt.h>
#include <system/memory.h>

#define WIDTH 128
#define HEIGHT 128
#define DEPTH 4

#include "data/rork-128.c"

static __code BitmapT *screen[2];
static __code CopListT *cp;
static __code CopInsPairT *bplptr;
static __code u_char *chunky;
static __code short active;
static __code volatile short c2p_phase;
static __code void **c2p_bpl;

/* [0 0 0 0 a0 a1 a2 a3] => [a0 a1 0 0 a2 a3 0 0] */
static const u_char PixelHi[16] = {
  0x00, 0x04, 0x08, 0x0c, 0x40, 0x44, 0x48, 0x4c,
  0x80, 0x84, 0x88, 0x8c, 0xc0, 0xc4, 0xc8, 0xcc,
};

static void ScrambleTexture(void) {
  u_char *src = texture_pixels;
  u_char *dst = texture_pixels;
  short n = texture_width * texture_height;

  while (--n >= 0) {
    *dst++ = PixelHi[*src++];
  }
}

typedef struct Corner {
  short x, y;
  short u, v;
} CornerT;

typedef struct Side {
  short dx, dy, du, dv;         // 12.4 format
  short dxdy, dudy, dvdy;       // 8.8 format
  short x, u, v;                // 8.8 format
  short ys, ye;                 // integer
} SideT;

static inline int part_int(int num, int shift) {
  int r = abs(num) >> shift;
  return num >= 0 ? r : -r;
}

static inline int part_frac(int num, int shift) {
  int r = abs(num) & ((1 << shift) - 1);
  return 10000 * r >> shift;
}

#define FRAC(n, k) part_int((n), (k)), part_frac((n), (k))

static void InitSide(SideT *s, CornerT *pa, CornerT *pb) {
  short y1 = (pb->y + 7) >> 4;
  short y0 = (pa->y + 7) >> 4;
  short dy = pb->y - pa->y;

  s->ys = y0;
  s->ye = y1;
  s->dy = dy;
  s->dx = pb->x - pa->x;
  s->du = pb->u - pa->u;
  s->dv = pb->v - pa->v;

#if 0
  Log("A: x: %d.%04d, y: %d.%04d, u: %d.%04d, v: %d.%04d\n",
      FRAC(pa->x, 4), FRAC(pa->y, 4), FRAC(pa->u, 4), FRAC(pa->v, 4));
  Log("B: x: %d.%04d, y: %d.%04d, u: %d.%04d, v: %d.%04d\n",
      FRAC(pb->x, 4), FRAC(pb->y, 4), FRAC(pb->u, 4), FRAC(pb->v, 4));
  Log("> dx: %d.%04d, dy: %d.%04d, du: %d.%04d, dv: %d.%04d\n",
      FRAC(s->dx, 4), FRAC(dy, 4), FRAC(s->du, 4), FRAC(s->dv, 4));
#endif

  if (s->ye > s->ys) {
    short prestep = ((pa->y + 7) & -16) + 8 - pa->y;

#if 0
    Log("> prestep: %d.%04d\n", FRAC(prestep, 4));
#endif

    s->dxdy = div16(s->dx << 8, dy);                    // 20.12 / 12.4 = 8.8
    s->x = (pa->x << 4) + (s->dxdy * prestep >> 4);     // 8.8 * 12.4 = 20.12

    s->dudy = div16(s->du << 8, dy);
    s->u = (pa->u << 4) + (s->dudy * prestep >> 4);

    s->dvdy = div16(s->dv << 8, dy);
    s->v = (pa->v << 4) + (s->dvdy * prestep >> 4);
  } else {
    s->dxdy = 0;
    s->x = pa->x << 4;

    s->dudy = 0;
    s->u = pa->u << 4;

    s->dvdy = 0;
    s->v = pa->v << 4;
  }

#if 0
    Log("> dxdy: %d.%04d, x: %d.%04d\n", FRAC(s->dxdy, 8), FRAC(s->x, 8));
    Log("> dudy: %d.%04d, u: %d.%04d\n", FRAC(s->dudy, 8), FRAC(s->u, 8));
    Log("> dvdy: %d.%04d, v: %d.%04d\n", FRAC(s->dvdy, 8), FRAC(s->v, 8));
#endif
}

static void DrawSpan(SideT *l, SideT *r, short du, short dv) {
  u_char *pixels = chunky;
  SideT *m = l;

  if ((r->x < l->x) ||
      (r->x == l->x && l->dxdy > r->dxdy)) {
    SideT *t = l; l = r; r = t;
  }

  pixels += m->ys * WIDTH;

  /* reduce to texture size => 9.7 */
  du >>= 1;
  dv >>= 1;

#if 0
  Log("*** ys: %d, ye: %d, du: %d.%04d, dv: %d.%04d\n",
      m->ys, m->ye, FRAC(du, 8), FRAC(dv, 8));
#endif

  while (m->ys < m->ye) {
    short xs, xe, prestep, u, v;

    xs = (l->x + 127) >> 8;
    xe = (r->x + 127) >> 8;

#if 0
    prestep = ((l->x + 7) & -16) - l->x + 8;

    u = l->u + (du * prestep >> 8);
    v = l->v + (dv * prestep >> 8);
#else
    (void)prestep;
    u = l->u >> 1;
    v = l->v >> 1;
#endif

#if 0
    Log("$ y: %d, xs: %d.%04d, xe: %d.%04d, u: %d.%04d, v: %d.%04d\n",
        m->ys, FRAC(l->x, 8), FRAC(r->x, 8), FRAC(u, 8), FRAC(v, 8));
#endif

    if (xe > xs) {
      short n = xe - xs - 1;
      register short mask asm("d5") = -128;
      u_char *line = &pixels[xs];

      do {
        register short ui asm("d6") = u >> 7;
        register short vi asm("d7") = v;
        short offset = ui + (vi & mask);

        *line++ = texture_pixels[offset];

        u += du;
        v += dv;
      } while (--n != -1);
    }

    l->x += l->dxdy;
    l->u += l->dudy;
    l->v += l->dvdy;
    r->x += r->dxdy;
    r->u += r->dudy;
    r->v += r->dvdy;

    m->ys++;
    pixels += WIDTH;
  }
}

static void DrawTriangle(CornerT *p0, CornerT *p1, CornerT *p2) {
  // sort them by y
  if (p0->y > p1->y) { CornerT *t = p1; p1 = p0; p0 = t; }
  if (p0->y > p2->y) { CornerT *t = p2; p2 = p0; p0 = t; }
  if (p1->y > p2->y) { CornerT *t = p2; p2 = p1; p1 = t; }

  {
    SideT s01, s02, s12;
    short du, dv;

    InitSide(&s01, p0, p1);
    InitSide(&s02, p0, p2);
    InitSide(&s12, p1, p2);

    {
      short u, v, x;

      if (s12.dy < s01.dy) {
        u = s02.dudy - s01.dudy;
        v = s02.dvdy - s01.dvdy;
        x = s02.dxdy - s01.dxdy;
      } else {
        u = s02.dudy - s12.dudy;
        v = s02.dvdy - s12.dvdy;
        x = s02.dxdy - s12.dxdy;
      }

      du = div16(u << 8, x);
      dv = div16(v << 8, x);
    }

    DrawSpan(&s01, &s02, du, dv);
    DrawSpan(&s12, &s02, du, dv);
  }
}

/* This is the size of a single bitplane in `screen`. */
#define BUFSIZE ((WIDTH / 8) * HEIGHT * 4) /* 8192 bytes */

/* If you think you can speed it up (I doubt it) please first look into
 * `c2p_2x1_4bpl_pixels_per_byte_blitter.py` in `prototypes/c2p`. */

#define C2P_LAST 12

static void ChunkyToPlanar(CustomPtrT custom_) {
  register void **bpl = c2p_bpl;

  /*
   * Our chunky buffer of size (WIDTH, HEIGHT) is stored in bpl[0] and bpl[1].
   * Each 32-bit long word of chunky buffer contains four pixels [a b c d]
   * in scrambled format described below.
   * Please note that a_i is the i-th least significant bit of a.
   *
   * [ a0 a1 -- -- a2 a3 -- -- | b0 b1 -- -- b2 b3 -- -- |
   *   c0 c1 -- -- c2 c3 -- -- | d0 d1 -- -- d2 d3 -- -- ]
   *
   * Chunky to planar is divided into the following major steps:
   * 
   * ...: TODO
   * ...: TODO
   * Swap 4x2: in two consecutive 16-bit words swap diagonally two bits,
   *           i.e. [b0 b1] <-> [c0 c1], [b2 b3] <-> [c2 c3].
   * Expand 2x1: [x0 x1 ...] is translated into [x0 x0 ...] and [x1 x1 ...]
   *             and copied to corresponding bitplanes, this step effectively
   *             stretches pixels to 2x1.
   *
   * Line doubling is performed using copper. Rendered bitmap will have size
   * (WIDTH*2, HEIGHT, DEPTH) and will be placed in bpl[2] and bpl[3].
   */

  custom_->intreq_ = INTF_BLIT;

  switch (c2p_phase) {
    case 0:
      /* Initialize chunky to planar. */
      custom_->bltafwm = -1;
      custom_->bltalwm = -1;

      custom_->bltamod = 0;
      custom_->bltbmod = 0;
      custom_->bltdmod = 0;
      custom_->bltcdat = 0x3300;

      custom_->bltapt = bpl[0] + BUFSIZE * 2 - 2;
      custom_->bltbpt = bpl[0] + BUFSIZE * 2 - 2;
      custom_->bltdpt = bpl[2] + BUFSIZE * 2 - 2;
      
      /* ((a << 6) & 0xCC00) | (b & ~0xCC00) */
      custom_->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC) | ASHIFT(6);
      custom_->bltcon1 = BLITREVERSE;

      /* overall size: BUFSIZE * 2 bytes (chunk buffer size) */
      custom_->bltsize = BLTSIZE(WIDTH / 2, HEIGHT);
      break; /* B */

    case 1:
      custom_->bltamod = 6;
      custom_->bltbmod = 6;
      custom_->bltdmod = 2;

      custom_->bltapt = bpl[2];
      custom_->bltbpt = bpl[2] + 4;
      custom_->bltdpt = bpl[0];
      custom_->bltcdat = 0xFF00;

      /* (a & 0xFF00) | ((b >> 8) & ~0xFF00) */
      custom_->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC);
      custom_->bltcon1 = BSHIFT(8);

    case 2:
      /* invoked twice, overall size: BUFSIZE / 2 bytes */
      custom_->bltsize = BLTSIZE(1, BUFSIZE / 8);
      break; /* C (even) */

    case 3:
      custom_->bltapt = bpl[2] + 2;
      custom_->bltbpt = bpl[2] + 6;
      custom_->bltdpt = bpl[0] + 2;

    case 4:
      /* invoked twice, overall size: BUFSIZE / 2 bytes */
      custom_->bltsize = BLTSIZE(1, BUFSIZE / 8);
      break; /* C (odd) */

    case 5:
      custom_->bltamod = 2;
      custom_->bltbmod = 2;
      custom_->bltdmod = 0;
      custom_->bltcdat = 0xF0F0;

      /* Swap 4x2, pass 1, high-bits. */
      custom_->bltapt = bpl[0];
      custom_->bltbpt = bpl[0] + 2;
      custom_->bltdpt = bpl[1] + BUFSIZE / 2;

      /* (a & 0xF0F0) | ((b >> 4) & ~0xF0F0) */
      custom_->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC);
      custom_->bltcon1 = BSHIFT(4);

    case 6:
      /* invoked twice, overall size: BUFSIZE / 2 bytes */
      custom_->bltsize = BLTSIZE(1, BUFSIZE / 8);
      break; /* D[0] */

    case 7:
      /* Swap 4x2, pass 2, low-bits. */
      custom_->bltapt = bpl[1] - 4;
      custom_->bltbpt = bpl[1] - 2;
      custom_->bltdpt = bpl[1] + BUFSIZE / 2 - 2;

      /* ((a << 4) & 0xF0F0) | (b & ~0xF0F0) */
      custom_->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC) | ASHIFT(4);
      custom_->bltcon1 = BLITREVERSE;

    case 8:
      /* invoked twice, overall size: BUFSIZE / 2 bytes */
      custom_->bltsize = BLTSIZE(1, BUFSIZE / 8);
      break;

    case 9:
      custom_->bltamod = 0;
      custom_->bltbmod = 0;
      custom_->bltdmod = 0;
      custom_->bltcdat = 0xAAAA;

      custom_->bltapt = bpl[1];
      custom_->bltbpt = bpl[1];
      custom_->bltdpt = bpl[3];

      /* (a & 0xAAAA) | ((b >> 1) & ~0xAAAA) */
      custom_->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC);
      custom_->bltcon1 = BSHIFT(1);

      /* overall size: BUFSIZE bytes */
      custom_->bltsize = BLTSIZE(4, BUFSIZE / 8);
      break;

    case 10:
      custom_->bltapt = bpl[1] + BUFSIZE - 2;
      custom_->bltbpt = bpl[1] + BUFSIZE - 2;
      custom_->bltdpt = bpl[2] + BUFSIZE - 2;
      custom_->bltcdat = 0xAAAA;

      /* ((a << 1) & 0xAAAA) | (b & ~0xAAAA) */
      custom_->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC) | ASHIFT(1);
      custom_->bltcon1 = BLITREVERSE;

      /* overall size: BUFSIZE bytes */
      custom_->bltsize = BLTSIZE(4, BUFSIZE / 8);
      break;

    case 11:
      CopInsSet32(&bplptr[0], bpl[2]);
      CopInsSet32(&bplptr[1], bpl[3]);
      CopInsSet32(&bplptr[2], bpl[2] + BUFSIZE / 2);
      CopInsSet32(&bplptr[3], bpl[3] + BUFSIZE / 2);

      /* clear the buffer */
      custom_->bltapt = texture_pixels;
      custom_->bltdpt = bpl[0];
      custom_->bltcon0 = (SRCA | DEST) | A_TO_D;
      custom_->bltcon1 = 0;

      /* overall size: BUFSIZE * 2 bytes (chunk buffer size) */
      custom_->bltsize = BLTSIZE(WIDTH / 2, HEIGHT);
      break;

    default:
      break;
  }

  c2p_phase++;
}

static CopListT *MakeCopperList(short active) {
  CopListT *cp = NewCopList(HEIGHT * 2 * 3 + 50);
  short i;

  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);
  for (i = 0; i < HEIGHT * 2; i++) {
    CopWaitSafe(cp, Y(i), 0);
    /* Line doubling. */
    CopMove16(cp, bpl1mod, (i & 1) ? 0 : -(WIDTH * 2) / 8);
    CopMove16(cp, bpl2mod, (i & 1) ? 0 : -(WIDTH * 2) / 8);
  }
  return CopListFinish(cp);
}

static void Init(void) {
  screen[0] = NewBitmap(WIDTH * 2, HEIGHT * 2, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH * 2, HEIGHT * 2, DEPTH, BM_CLEAR);

  ScrambleTexture();

  SetupPlayfield(MODE_LORES, DEPTH, X(32), Y(0), WIDTH * 2, HEIGHT * 2);
  LoadColors(texture_colors, 0);

  cp = MakeCopperList(0);
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER | DMAF_BLITTER);

  active = 0;
  c2p_bpl = NULL;
  c2p_phase = 256;

  SetIntVector(INTB_BLIT, (IntHandlerT)ChunkyToPlanar, (void *)custom);
  EnableINT(INTF_BLIT);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_BLITTER);

  DisableINT(INTF_BLIT);
  ResetIntVector(INTB_BLIT);

  DeleteCopList(cp);

  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

PROFILE(DrawTriangle);

#define RADIUS_X fx4i(WIDTH / 2 - 2)
#define RADIUS_Y fx4i(HEIGHT / 2 - 2)
#define MIDDLE_X fx4i(WIDTH / 2)
#define MIDDLE_Y fx4i(HEIGHT / 2)

#define STEP 4
#define PHASE 0

static void Render(void) {
  CornerT p0, p1, p2;

  p0.x = normfx(SIN(frameCount * STEP + PHASE) * RADIUS_X) + MIDDLE_X;
  p0.y = normfx(COS(frameCount * STEP + PHASE) * RADIUS_Y) + MIDDLE_Y;
  p0.u = fx4i(0);
  p0.v = fx4i(0);

  p1.x = normfx(SIN(frameCount * STEP + PHASE + 0x555) * RADIUS_X) + MIDDLE_X;
  p1.y = normfx(COS(frameCount * STEP + PHASE + 0x555) * RADIUS_Y) + MIDDLE_Y;
  p1.u = fx4i(texture_width - 1);
  p1.v = fx4i(0);

  p2.x = normfx(SIN(frameCount * STEP + PHASE + 0xaaa) * RADIUS_X) + MIDDLE_X;
  p2.y = normfx(COS(frameCount * STEP + PHASE + 0xaaa) * RADIUS_Y) + MIDDLE_Y;
  p2.u = fx4i(0);
  p2.v = fx4i(texture_height - 1);

  ProfilerStart(DrawTriangle);
  {
    chunky = screen[active]->planes[0];
    DrawTriangle(&p0, &p1, &p2);
  }
  ProfilerStop(DrawTriangle);

  while (c2p_phase < C2P_LAST)
    WaitBlitter();

  c2p_phase = 0;
  c2p_bpl = screen[active]->planes;
  ChunkyToPlanar(custom);
  active ^= 1;
}

EFFECT(TexTri, NULL, NULL, Init, Kill, Render, NULL);
