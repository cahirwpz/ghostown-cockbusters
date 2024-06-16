#include "effect.h"
#include "blitter.h"
#include "copper.h"
#include "3d.h"
#include "fx.h"

#define WIDTH  256
#define HEIGHT 256
#define DEPTH  4

static Object3D *cube;
static CopListT *cp;
static CopInsPairT *bplptr;
static BitmapT *screen[2];
static BitmapT *buffer;
static int active = 0;

#include "data/flatshade-pal.c"
#include "data/codi.c"

static void Init(void) {
  cube = NewObject3D(&codi);
  cube->translate.z = fx4i(-250);

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  buffer = NewBitmap(WIDTH, HEIGHT, 1, 0);

  SetupPlayfield(MODE_LORES, DEPTH, X(32), Y(0), WIDTH, HEIGHT);
  LoadColors(flatshade_colors, 0);

  cp = NewCopList(80);
  bplptr = CopSetupBitplanes(cp, screen[0], DEPTH);
  CopListFinish(cp);
  CopListActivate(cp);
  EnableDMA(DMAF_BLITTER | DMAF_RASTER | DMAF_BLITHOG);
}

static void Kill(void) {
  DisableDMA(DMAF_RASTER);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
  DeleteBitmap(buffer);
  DeleteCopList(cp);
  DeleteObject3D(cube);
}

#define MULVERTEX1(D, E) {                      \
  short t0 = (*v++) + y;                        \
  short t1 = (*v++) + x;                        \
  int t2 = (*v++) * z;                          \
  v++;                                          \
  D = ((t0 * t1 + t2 - x * y) >> 4) + E;        \
}

#define MULVERTEX2(D) {                         \
  short t0 = (*v++) + y;                        \
  short t1 = (*v++) + x;                        \
  int t2 = (*v++) * z;                          \
  short t3 = (*v++);                            \
  D = normfx(t0 * t1 + t2 - x * y) + t3;        \
}

static void TransformVertices(Object3D *object) {
  Matrix3D *M = &object->objectToWorld;
  short *mx = (short *)M;
  short *src = (short *)object->point;
  short *dst = (short *)object->vertex;
  register short n asm("d7") = object->vertices - 1;

  int m0 = (M->x << 8) - ((M->m00 * M->m01) >> 4);
  int m1 = (M->y << 8) - ((M->m10 * M->m11) >> 4);

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
    if (((Point3D *)src)->flags) {
      short x = *src++;
      short y = *src++;
      short z = *src++;
      short *v = mx;
      int xp, yp;
      short zp;

      *src++ = 0; /* reset flags */

      MULVERTEX1(xp, m0);
      MULVERTEX1(yp, m1);
      MULVERTEX2(zp);

      *dst++ = div16(xp, zp) + WIDTH / 2;  /* div(xp * 256, zp) */
      *dst++ = div16(yp, zp) + HEIGHT / 2; /* div(yp * 256, zp) */
      *dst++ = zp;
      dst++;
    } else {
      src += 4;
      dst += 4;
    }
  } while (--n != -1);
}

static void DrawObject(Object3D *object, CustomPtrT custom_ asm("a6")) {
  SortItemT *item = object->visibleFace;
  void *planes = buffer->planes[0];

  void *_objdat = object->objdat;
  void *_vertex = object->vertex;

  custom_->bltafwm = -1;
  custom_->bltalwm = -1;

  for (; item->index >= 0; item++) {
    short minX = 32767;
    short minY = 32767;
    short maxX = -32768;
    short maxY = -32768;

    /* Draw edges and calculate bounding box. */
    {
      register short *index asm("a3") = (short *)(FACE(item->index)->indices);
      register short m asm("d7") = FACE(item->index)->count - 1;

      do {
        /* Calculate area. */
        {
          short i = *index++; /* vertex */
          short x = VERTEX(i)->x;
          short y = VERTEX(i)->y;

          if (x < minX)
            minX = x;
          if (x > maxX)
            maxX = x;

          if (y < minY)
            minY = y;
          if (y > maxY)
            maxY = y;
        }

        /* Draw edge. */
        {
          short bltcon0, bltcon1, bltsize, bltbmod, bltamod;
          int bltapt, bltcpt;
          short x0, y0, x1, y1;
          short dmin, dmax, derr;
            
          {
            short j = *index++; /* edge */
            short i;

            i = EDGE(j)->point[0];
            x0 = VERTEX(i)->x;
            y0 = VERTEX(i)->y;

            i = EDGE(j)->point[1];
            x1 = VERTEX(i)->x;
            y1 = VERTEX(i)->y;
          }

          if (y0 == y1)
            goto next;

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

          bltcpt = (int)planes + (short)(((y0 << 5) + (x0 >> 3)) & ~1);

          bltcon0 = rorw(x0 & 15, 4) | BC0F_LINE_EOR;
          bltcon1 |= rorw(x0 & 15, 4);

          dmin <<= 1;
          derr = dmin - dmax;

          bltamod = derr - dmax;
          bltbmod = dmin;
          bltsize = (dmax << 6) + 66;
          bltapt = derr;

          _WaitBlitter(custom_);

          custom_->bltbdat = 0xffff;
          custom_->bltadat = 0x8000;
          custom_->bltcon0 = bltcon0;
          custom_->bltcon1 = bltcon1;
          custom_->bltcpt = (void *)bltcpt;
          custom_->bltapt = (void *)bltapt;
          custom_->bltdpt = planes;
          custom_->bltcmod = WIDTH / 8;
          custom_->bltbmod = bltbmod;
          custom_->bltamod = bltamod;
          custom_->bltdmod = WIDTH / 8;
          custom_->bltsize = bltsize;
        }
next:
      } while (--m != -1);
    }

    {
      short bltstart, bltend;
      u_short bltmod, bltsize;

      /* Align to word boundary. */
      minX = (minX & ~15) >> 3;
      /* to avoid case where a line is on right edge */
      maxX = ((maxX + 16) & ~15) >> 3;

      {
        short w = maxX - minX;
        short h = maxY - minY + 1;

        bltstart = minX + minY * (WIDTH / 8);
        bltend = maxX + maxY * (WIDTH / 8) - 2;
        bltsize = (h << 6) | (w >> 1);
        bltmod = (WIDTH / 8) - w;
      }

      /* Fill face. */
      {
        void *src = planes + bltend;

        _WaitBlitter(custom_);

        custom_->bltcon0 = (SRCA | DEST) | A_TO_D;
        custom_->bltcon1 = BLITREVERSE | FILL_XOR;
        custom_->bltapt = src;
        custom_->bltdpt = src;
        custom_->bltamod = bltmod;
        custom_->bltbmod = bltmod;
        custom_->bltdmod = bltmod;
        custom_->bltsize = bltsize;
      }

      /* Copy filled face to screen. */
      {
        void **dstbpl = &screen[active]->planes[DEPTH];
        void *src = planes + bltstart;
        char mask = 1 << (DEPTH - 1);
        char color = FACE(item->index)->flags;
        short n = DEPTH;

        while (--n >= 0) {
          void *dst = *(--dstbpl) + bltstart;
          u_short bltcon0;

          if (color & mask)
            bltcon0 = (SRCA | SRCB | DEST) | A_OR_B;
           else
            bltcon0 = (SRCA | SRCB | DEST) | (NABC | NABNC);

          _WaitBlitter(custom_);

          custom_->bltcon0 = bltcon0;
          custom_->bltcon1 = 0;
          custom_->bltapt = src;
          custom_->bltbpt = dst;
          custom_->bltdpt = dst;
          custom_->bltsize = bltsize;

          mask >>= 1;
        }
      }

      /* Clear working area. */
      {
        void *data = planes + bltstart;

        _WaitBlitter(custom_);

        custom_->bltcon0 = (DEST | A_TO_D);
        custom_->bltadat = 0;
        custom_->bltdpt = data;
        custom_->bltsize = bltsize;
      }
    }
  }
}

static void BitmapClearFast(BitmapT *dst) {
  u_short height = (short)dst->height * (short)dst->depth;
  u_short bltsize = (height << 6) | (dst->bytesPerRow >> 1);
  void *bltpt = dst->planes[0];

  WaitBlitter();

  custom->bltcon0 = DEST | A_TO_D;
  custom->bltcon1 = 0;
  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltadat = 0;
  custom->bltdmod = 0;
  custom->bltdpt = bltpt;
  custom->bltsize = bltsize;
}

PROFILE(Transform);
PROFILE(Draw);

static void Render(void) {
  BitmapClearFast(screen[active]);

  ProfilerStart(Transform);
  {
    cube->rotate.x = cube->rotate.y = cube->rotate.z = frameCount * 8;
    UpdateObjectTransformation(cube);
    UpdateFaceVisibility(cube);
    UpdateVertexVisibility(cube);
    TransformVertices(cube);
    SortFaces(cube);
  }
  ProfilerStop(Transform); // Average: 163

  ProfilerStart(Draw);
  {
    DrawObject(cube, custom);
  }
  ProfilerStop(Draw); // Average: 671

  CopUpdateBitplanes(bplptr, screen[active], DEPTH);
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(FlatShade, NULL, NULL, Init, Kill, Render, NULL);
