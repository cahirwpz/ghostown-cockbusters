#include "effect.h"
#include "blitter.h"
#include "copper.h"
#include "3d.h"
#include "fx.h"

#define WIDTH  256
#define HEIGHT 256
#define DEPTH 3

#define screen_bplSize (WIDTH * HEIGHT / 8)

#define TZ (-256)

static Object3D *cube;
static CopListT *cp;
static BitmapT *screen[2];
static CopInsPairT *bplptr;
static int active = 0;

#include "data/flares32.c"
#include "data/dna2.c"
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
    short i, j;

    for (i = 0; i < carrion_height; i++) {
      short bgcol = *data++;
      /* Start exchanging palette colors at the end of previous line. */
      CopWaitSafe(cp, VP(i-1), HP(320 - 32 - 4));
      CopMove16(cp, color[0], 0);

      for (j = 1; j < carrion_cols_width; j++) {
        CopMove16(cp, color[j + 8], *data++);
      }

      CopWaitSafe(cp, VP(i), HP(24));
      CopMove16(cp, color[0], bgcol);
    }
  }

  return CopListFinish(cp);
}
static void Init(void) {
  cube = NewObject3D(&dna_helix);
  cube->translate.z = fx4i(TZ);

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);

  SetupDisplayWindow(MODE_LORES, X(32), Y(0), WIDTH, HEIGHT);
  SetupBitplaneFetch(MODE_LORES, X(32), WIDTH);
  SetupMode(MODE_DUALPF, DEPTH + carrion_depth);
  LoadColors(bobs_colors, 0);

  /* reverse playfield priorities */
  custom->bplcon2 = 0;
  /* set up bplmod for both playfieds */ 
  custom->bpl1mod = 0;
  custom->bpl2mod = 0;

  cp = MakeCopperList();
  CopListActivate(cp);
  EnableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);
}

static void Kill(void) {
  DeleteCopList(cp);
  DisableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
  DeleteObject3D(cube);
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

#define BOBW 48
#define BOBH 32

static void DrawFlares(Object3D *object, void *src, void *dst,
                       CustomPtrT custom_ asm("a6"))
{
  void *_objdat = object->objdat;
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

      x -= 16;
      y -= 16;

      z >>= 4;
      z -= TZ;
      z += 128 - 32;
      z = z + z + z - 32;
      z &= ~31;

      if (z < 0)
        z = 0;
      else if (z > bobs_height - BOBH)
        z = bobs_height - BOBH;

      {
        const short bltshift = rorw(x & 15, 4) | (SRCA | SRCB | DEST) | A_OR_B;
        const short bltsize = (BOBH << 6) | (BOBW / 16);

        short src_start = (z + z + z) + (z + z + z);
        short dst_start = ((x & ~15) >> 3) + (y << 5);

        void *apt = src + src_start;
        void *dpt = dst + dst_start;

        _WaitBlitter(custom_);

        custom_->bltcon0 = bltshift;
        custom_->bltapt = apt;
        custom_->bltbpt = dpt;
        custom_->bltdpt = dpt;
        custom_->bltsize = bltsize;

        apt += bobs_bplSize;
        dpt += screen_bplSize;

        _WaitBlitter(custom_);

        custom_->bltapt = apt;
        custom_->bltbpt = dpt;
        custom_->bltdpt = dpt;
        custom_->bltsize = bltsize;

        apt += bobs_bplSize;
        dpt += screen_bplSize;

        _WaitBlitter(custom_);

        custom_->bltapt = apt;
        custom_->bltbpt = dpt;
        custom_->bltdpt = dpt;
        custom_->bltsize = bltsize;
      }
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
  custom_->bltcmod = WIDTH / 8;
  custom_->bltdmod = WIDTH / 8;

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

      bltcpt = (int)dst + (short)(((y0 << 5) + (x0 >> 3)) & ~1);

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
      custom_->bltdpt = dst + screen_bplSize * 2;
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
    cube->rotate.x = cube->rotate.y = cube->rotate.z = frameCount * 12;

    UpdateObjectTransformation(cube);
    TransformVertices(cube);
  }
  ProfilerStop(TransformObject);

  WaitBlitter();

  ProfilerStart(DrawObject);
  {
    DrawFlares(cube, bobs.planes[0], screen[active]->planes[0], custom);
    DrawLinks(cube, screen[active]->planes[1], custom);
  }
  ProfilerStop(DrawObject);

  TaskWaitVBlank();

  CopInsSet32(&bplptr[0], screen[active]->planes[0]);
  CopInsSet32(&bplptr[2], screen[active]->planes[1]);
  CopInsSet32(&bplptr[4], screen[active]->planes[2]);
  active ^= 1;
}

EFFECT(Bobs3D, NULL, NULL, Init, Kill, Render, NULL);
