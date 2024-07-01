#include "effect.h"
#include "blitter.h"
#include "copper.h"
#include "3d.h"
#include "fx.h"
#include "debug.h"

#define WIDTH  256
#define HEIGHT 256
#define DEPTH 3

#define screen_bplSize (WIDTH * HEIGHT / 8)

#define TZ (-256)

static Object3D *object;
static CopListT *cp;
static BitmapT *screen[2];
static CopInsPairT *bplptr;
static int active = 0;

#include "data/flares32.c"
#include "data/dna.c"
#include "data/carrion-metro-pal.c"
#include "data/carrion-metro-data.c"

static CopListT *MakeCopperList(void) {
  CopListT *cp = NewCopList(100 + carrion_height * (carrion_cols_width + 3));

  /* interleaved bitplanes setup */
  CopWait(cp, Y(-1), 0);

  bplptr = CopMove32(cp, bplpt[0], screen[1]->planes[0]);
  CopMove32(cp, bplpt[1], carrion.planes[0]);
  CopMove32(cp, bplpt[2], screen[1]->planes[1]);
  CopMove32(cp, bplpt[3], carrion.planes[1]);
  CopMove32(cp, bplpt[4], screen[1]->planes[2]);

  {
    u_short *data = carrion_cols_pixels;
    short i;

    for (i = 0; i < carrion_height; i++) {
      short bgcol = *data++;

      /* Start exchanging palette colors at the end of previous line. */
      CopWaitSafe(cp, Y(i-1), HP(320 - 32 - 4));
      CopMove16(cp, color[0], 0);

      CopWaitSafe(cp, Y(i), HP(0));
      CopMove16(cp, color[9], *data++);
      CopMove16(cp, color[10], *data++);
      CopMove16(cp, color[11], *data++);
      CopMove16(cp, color[0], bgcol);
    }
  }

  return CopListFinish(cp);
}
static void Init(void) {
  object = NewObject3D(&dna_helix);
  object->translate.z = fx4i(TZ);

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR|BM_INTERLEAVED);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR|BM_INTERLEAVED);

  SetupDisplayWindow(MODE_LORES, X(32), Y(0), WIDTH, HEIGHT);
  SetupBitplaneFetch(MODE_LORES, X(32), WIDTH);
  SetupMode(MODE_DUALPF, DEPTH + carrion_depth);
  LoadColors(bobs_colors, 0);

  /* reverse playfield priorities */
  custom->bplcon2 = 0;
  /* bitplane modulos for both playfields */
  custom->bpl1mod = WIDTH / 8 * (DEPTH - 1);
  custom->bpl2mod = WIDTH / 8 * (carrion_depth - 1);

  cp = MakeCopperList();
  CopListActivate(cp);
  EnableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);
}

static void Kill(void) {
  DeleteCopList(cp);
  DisableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
  DeleteObject3D(object);
}

#define MULVERTEX1(D, E) {              \
  short t0 = (*v++) + y;                \
  short t1 = (*v++) + x;                \
  int t2 = (*v++) * z;                  \
  v++;                                  \
  D = ((t0 * t1 + t2 - xy) >> 4) + E;   \
}

#define MULVERTEX2(D) {                 \
  short t0 = (*v++) + y;                \
  short t1 = (*v++) + x;                \
  int t2 = (*v++) * z;                  \
  short t3 = (*v++);                    \
  D = normfx(t0 * t1 + t2 - xy) + t3;   \
}

static void TransformVertices(Object3D *object) {
  Matrix3D *M = &object->objectToWorld;
  void *_objdat = object->objdat;
  short *group = object->vertexGroups;

  int m0 = (M->x - normfx(M->m00 * M->m01)) << 8;
  int m1 = (M->y - normfx(M->m10 * M->m11)) << 8;
  /* WARNING! This modifies camera matrix! */
  M->z -= normfx(M->m20 * M->m21);

  /*
   * A = m00 * m01
   * B = m10 * m11
   * C = m20 * m21 
   * yx = y * x
   *
   * (m00 + y) * (m01 + x) + m02 * z - yx + (mx - A)
   * (m10 + y) * (m11 + x) + m12 * z - yx + (my - B)
   * (m20 + y) * (m21 + x) + m22 * z - yx + (mz - C)
   */

  do {
    short i;

    while ((i = *group++)) {
      short *v = (short *)M;
      short *pt = (short *)POINT(i);

      short x = *pt++;
      short y = *pt++;
      short z = *pt++;
      short zp;

      int xy = x * y;
      int xp, yp;

      MULVERTEX1(xp, m0);
      MULVERTEX1(yp, m1);
      MULVERTEX2(zp);

      *pt++ = div16(xp, zp) + WIDTH / 2;  /* div(xp * 256, zp) */
      *pt++ = div16(yp, zp) + HEIGHT / 2; /* div(yp * 256, zp) */
      *pt++ = zp;
    }
  } while (*group);
}

#define POINTS_PER_TURN 10
#define TURNS 4
#define RADIUS 2.5

#define NPOINTS (POINTS_PER_TURN * TURNS)

static void GenCircularDoubleHelix(Node3D *node, short phi_offset) {
  register short radius asm("a3") = fx12f(2.5);
  u_short alpha; /* 0..65535 */
  short i;

  for (i = 0, alpha = 0; i < NPOINTS; i++, alpha += 65536 / NPOINTS) {
    /* 
     * 1) both theta and pi are in Q4.12 format
     * 2) target x, y, z are in Q12.4 format
     * 3) x, y, z need to be multiplied by 32 before casting to target format
     */

    /* theta = 2 * pi * i / points */
    short theta = alpha >> 4;
    /* phi = turns * 2 * pi * i / points */
    short phi = alpha / TURNS + phi_offset;

    short sin_theta = SIN(theta);
    short cos_theta = COS(theta);

    /* helix_radius is 0.5, so shift right by 1 */
    short sin_phi = SIN(phi) >> 1;
    short cos_phi = COS(phi) >> 1;

    sin_theta += sin_theta;
    cos_theta += cos_theta;

    {
      short *p = &node[0].point.x; node++;

      /* for x / y:
       *  - 8.24 * 32 => 13.19 (for free)
       *  - 13.19 << 1 => 12.20 (done above)
       *  - swap16(12.20) => 12.4
       * for z:
       *  - 4.12 * 32 => 9.7 (for free)
       *  - 9.7 >> 3 => 12.4
       */

      /* x = (radius + helix_radius * cos(phi)) * cos(theta) */
      *p++ = swap16((short)(radius + cos_phi) * cos_theta);
      /* y = (radius + helix_radius * sin(phi)) * sin(theta) */
      *p++ = swap16((short)(radius + sin_phi) * sin_theta);
      /* z = helix_radius * sin(phi) */
      *p++ = sin_phi >> 3;
    }

#if 0
    phi += SIN_HALF_PI;
    cos_phi = COS(phi) >> 1;
    sin_phi = SIN(phi) >> 1;
#else
    {
      short _cos_phi = cos_phi;
      cos_phi = -sin_phi;
      sin_phi = _cos_phi;
    }
#endif

    {
      short *p = &node[0].point.x; node++;

      /* (radius + helix_radius * cos(phi)) * cos(theta) */
      *p++ = swap16((short)(radius + cos_phi) * cos_theta);
      /* (radius + helix_radius * sin(phi)) * sin(theta) */
      *p++ = swap16((short)(radius + sin_phi) * sin_theta);
      /* helix_radius * sin(phi) */
      *p++ = sin_phi >> 3;
    }
  }
}

#define BOBW 48
#define BOBH 32

typedef struct BobDesc {
  short offset;
  short align;
  short bltsize;
  short pad;
} BobDescT;

#define BOBDESC(Y, H)                                           \
  (BobDescT){                                                   \
    .offset = (Y) * (BOBW / 8) * DEPTH,                         \
    .align = -(H) / 2,                                          \
    .bltsize = ((H) * DEPTH << 6) | (BOBW / 16),                \
    .pad = 0,                                                   \
  }

static BobDescT bobdesc[16] = {
  BOBDESC(16, 1),
  BOBDESC(47, 3),
  BOBDESC(78, 5),
  BOBDESC(109, 7),
  BOBDESC(140, 9),
  BOBDESC(171, 11),
  BOBDESC(202, 13),
  BOBDESC(233, 15),
  BOBDESC(264, 17),
  BOBDESC(295, 19),
  BOBDESC(326, 21),
  BOBDESC(357, 23),
  BOBDESC(388, 25),
  BOBDESC(419, 27),
  BOBDESC(450, 29),
  BOBDESC(481, 31),
};

#define MAXZ -2048
#define MINZ -6144

static void DrawFlares(Object3D *object, void *src, void *dst,
                       CustomPtrT custom_ asm("a6"))
{
  void *_objdat = object->objdat;
  void *_bobdesc = bobdesc;
  short *group = object->vertexGroups;

  _WaitBlitter(custom_);

  custom_->bltcon1 = 0;
  custom_->bltafwm = -1;
  custom_->bltalwm = -1;
  custom_->bltamod = 0;
  custom_->bltbmod = (WIDTH - BOBW) / 8;
  custom_->bltdmod = (WIDTH - BOBW) / 8;

  do {
    short v;

    while ((v = *group++)) {
      short *data = (short *)VERTEX(v);
      short x = *data++;
      short y = *data++;
      short z = *data++;

      const short bltshift = rorw(x & 15, 4) | (SRCA | SRCB | DEST) | A_OR_B;
      short bltsize;

      void *apt = src;
      void *dpt = dst;

      x -= 16;

      z -= MINZ;
      z >>= 5;
      z &= ~7;

      {
        short *desc = (short *)(_bobdesc + z);

        apt += *desc++;
        y += *desc++;
        bltsize = *desc++;
      }

#if 1
      y <<= 5;
      y += y + y;     /* y *= 96 */
      y += (x & ~15) >> 3;
      dpt += y;
#else
      dpt += (x & ~15) >> 3;
      dpt += y * (WIDTH / 8) * DEPTH;
#endif

      _WaitBlitter(custom_);

      custom_->bltcon0 = bltshift;
      custom_->bltapt = apt;
      custom_->bltbpt = dpt;
      custom_->bltdpt = dpt;
      custom_->bltsize = bltsize;
    }
  } while (*group);
}

static void DrawLinks(Object3D *object, void *dst,
                      CustomPtrT custom_ asm("a6"))
{
  void *_objdat = object->objdat;
  short *group = object->faceGroups;

  _WaitBlitter(custom_);
  custom_->bltafwm = -1;
  custom_->bltalwm = -1;
  custom_->bltadat = 0x8000;
  custom_->bltbdat = 0xffff; /* Line texture pattern. */
  custom_->bltcmod = WIDTH / 8 * DEPTH;
  custom_->bltdmod = WIDTH / 8 * DEPTH;

  do {
    short f;

    while ((f = *group++)) {
      short bltcon0, bltcon1, bltsize, bltbmod, bltamod;
      int bltapt, bltcpt;
      short x0, y0, x1, y1;
      short dmin, dmax, derr;

      {
        short *vi = (short *)&FACE(f)->indices[0].vertex;
        short *pt;
        short i;

        i = *vi++;
        pt = (short *)VERTEX(i);
        x0 = *pt++;
        y0 = *pt++;

        i = *vi++;
        pt = (short *)VERTEX(i);
        x1 = *pt++;
        y1 = *pt++;
      }

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
          bltcon1 = AUL | SUD | LINEMODE;
        else
          bltcon1 = SUD | LINEMODE;
      } else {
        if (x0 >= x1)
          bltcon1 = SUL | LINEMODE;
        else
          bltcon1 = LINEMODE;
        swapr(dmax, dmin);
      }

      bltcpt = (int)dst + (short)(((y0 << 5) * DEPTH + (x0 >> 3)) & ~1);

      bltcon0 = rorw(x0 & 15, 4) | BC0F_LINE_OR;
      bltcon1 |= rorw(x0 & 15, 4);

      dmin <<= 1;
      derr = dmin - dmax;

      bltamod = derr - dmax;
      bltbmod = dmin;
      bltsize = (dmax << 6) + 66;
      bltapt = derr;

      _WaitBlitter(custom_);
      custom_->bltcon0 = bltcon0;
      custom_->bltcon1 = bltcon1;
      custom_->bltcpt = (void *)bltcpt;
      custom_->bltapt = (void *)bltapt;
      custom_->bltdpt = (void *)bltcpt;
      custom_->bltbmod = bltbmod;
      custom_->bltamod = bltamod;
      custom_->bltsize = bltsize;
    }
  } while (*group);
}

static void BitmapClearI(BitmapT *bm) {
  WaitBlitter();

  custom->bltadat = 0;
  custom->bltcon0 = DEST | A_TO_D;
  custom->bltcon1 = 0;
  custom->bltdmod = 0;
  custom->bltdpt = bm->planes[0];
  custom->bltsize = ((bm->height * bm->depth) << 6) | (bm->bytesPerRow >> 1);
}

PROFILE(TransformObject);
PROFILE(DrawObject);

static void Render(void) {
  BitmapClearI(screen[active]);

  ProfilerStart(TransformObject);
  {
    object->rotate.x = object->rotate.y = object->rotate.z = frameCount * 6;

    UpdateObjectTransformation(object);
    GenCircularDoubleHelix(object->objdat, frameCount * 24);
    TransformVertices(object);
  }
  ProfilerStop(TransformObject);

  WaitBlitter();

  ProfilerStart(DrawObject);
  {
    DrawFlares(object, bobs.planes[0], screen[active]->planes[0], custom);
    DrawLinks(object, screen[active]->planes[1], custom);
  }
  ProfilerStop(DrawObject);

  TaskWaitVBlank();

  CopInsSet32(&bplptr[0], screen[active]->planes[0]);
  CopInsSet32(&bplptr[2], screen[active]->planes[1]);
  CopInsSet32(&bplptr[4], screen[active]->planes[2]);
  active ^= 1;
}

EFFECT(Bobs3D, NULL, NULL, Init, Kill, Render, NULL);
