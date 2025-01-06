#include "effect.h"
#include "blitter.h"
#include "copper.h"
#include "3d.h"
#include "fx.h"
#include "debug.h"
#include "sync.h"

#define WIDTH  256
#define HEIGHT 256
#define DEPTH 3

#define screen_bplSize (WIDTH * HEIGHT / 8)
#define bobs_bpl_section __section(".datachip.bobs")
#define bobs2_bpl_section __section(".datachip.bobs2")

#include "data/dna3d.c"
#include "data/bobs.c"
#include "data/bobs2.c"
#include "data/bobs_gradient.c"
#include "data/bobs_gradient2.c"
#include "data/dna.c"
#include "data/necrocoq-00-data.c"
#include "data/necrocoq-00-pal.c"
#include "data/necrocoq-01.c"
#include "data/necrocoq-02.c"
#include "data/necrocoq-03.c"
#include "data/necrocoq-04.c"
#include "data/necrocoq-05.c"
#include "data/necrocoq-06.c"
#include "data/necrocoq-07.c"
#include "data/necrocoq-08.c"
#include "data/necrocoq-09.c"
#include "data/necrocoq-10.c"

#include "data/necrocoq2-00.c"
#include "data/necrocoq2-01.c"
#include "data/necrocoq2-02.c"
#include "data/necrocoq2-03.c"
#include "data/necrocoq2-04.c"
#include "data/necrocoq2-05.c"
#include "data/necrocoq2-06.c"
#include "data/necrocoq2-07.c"
#include "data/necrocoq2-08.c"
#include "data/necrocoq2-09.c"
#include "data/necrocoq2-10.c"

#include "data/fadein00.c"
#include "data/fadein01.c"
#include "data/fadein02.c"
#include "data/fadein03.c"
#include "data/fadein04.c"
#include "data/fadein05.c"
#include "data/fadein06.c"
#include "data/fadein07.c"
#include "data/fadein08.c"

#include "data/tearing-table.c"

// For glitches
static __code short hPos = 0;

static __code Object3D *object;
static __code CopListT *cp;
static __code BitmapT *screen[2];
static __code CopInsPairT *bplptr;
static CopInsT *linecol[necrocoq_height];
static CopInsT *bobscol[necrocoq_height];
static __code int active = 0;

static __code u_short *necrochicken_cols[11] = {
  necrocoq_00_cols_pixels,
  necrocoq_01_cols_pixels,
  necrocoq_02_cols_pixels,
  necrocoq_03_cols_pixels,
  necrocoq_04_cols_pixels,
  necrocoq_05_cols_pixels,
  necrocoq_06_cols_pixels,
  necrocoq_07_cols_pixels,
  necrocoq_08_cols_pixels,
  necrocoq_09_cols_pixels,
  necrocoq_10_cols_pixels,
};

static __code u_short *necrochicken2_cols[11] = {
  necrocoq2_00_cols_pixels,
  necrocoq2_01_cols_pixels,
  necrocoq2_02_cols_pixels,
  necrocoq2_03_cols_pixels,
  necrocoq2_04_cols_pixels,
  necrocoq2_05_cols_pixels,
  necrocoq2_06_cols_pixels,
  necrocoq2_07_cols_pixels,
  necrocoq2_08_cols_pixels,
  necrocoq2_09_cols_pixels,
  necrocoq2_10_cols_pixels,
};

static __code u_short *fadein_cols[9] = {
  fadein08_cols_pixels,
  fadein07_cols_pixels,
  fadein06_cols_pixels,
  fadein05_cols_pixels,
  fadein04_cols_pixels,
  fadein03_cols_pixels,
  fadein02_cols_pixels,
  fadein01_cols_pixels,
  fadein00_cols_pixels,
};


#define envelope_length 40

static __code short envelope[envelope_length] = {
  0, 0,
  1, 1,
  2, 2,
  3, 3,
  4, 4,
  5, 5,
  6, 6,
  7, 7,
  8, 8,
  9, 9,
  10, 10,
  9, 9,
  8, 8,
  7, 7,
  6, 6,
  5, 5,
  4, 4,
  3, 3,
  2, 2,
  1, 1,
};

static __code short envelope2[envelope_length-8] = {
  0, 0,
  1, 1,
  2, 2,
  3, 3,
  4, 4,
  5, 5,
  6, 6,
  7, 7,
  8, 8,
  7, 7,
  6, 6,
  5, 5,
  4, 4,
  3, 3,
  2, 2,
  1, 1,
};

static void BitmapClearI(BitmapT *bm) {
  WaitBlitter();

  custom->bltadat = 0;
  custom->bltcon0 = DEST | A_TO_D;
  custom->bltcon1 = 0;
  custom->bltdmod = 0;
  custom->bltdpt = bm->planes[0];
  custom->bltsize = ((bm->height * bm->depth) << 6) | (bm->bytesPerRow >> 1);
}

static CopListT *MakeCopperList(void) {
  CopListT *cp = 
    NewCopList(100 + necrocoq_height * (necrocoq_00_cols_width + 10));

  /* interleaved bitplanes setup */
  CopWait(cp, Y(-1), 0);

  bplptr = CopMove32(cp, bplpt[0], screen[1]->planes[0]);
  CopMove32(cp, bplpt[1], necrocoq.planes[0]);
  CopMove32(cp, bplpt[2], screen[1]->planes[1]);
  CopMove32(cp, bplpt[3], necrocoq.planes[1]);
  CopMove32(cp, bplpt[4], screen[1]->planes[2]);

  {
    u_short *pf1_data = bobs_cols_pixels;
    short k = IsAGA() ? 8 : 0;
    short i;

    for (i = 0; i < necrocoq_height; i++) {
      short fgcol;

      /* Start exchanging palette colors at the end of previous line. */
      CopWaitSafe(cp, Y(i-1), HP(320 - 32 - 4));
      fgcol = *pf1_data++;
      bobscol[i] = CopMove16(cp, color[1], fgcol);
      fgcol = *pf1_data++;
      CopMove16(cp, color[2], fgcol);
      CopMove16(cp, color[3], fgcol);
      fgcol = *pf1_data++;
      CopMove16(cp, color[4], fgcol);
      CopMove16(cp, color[5], fgcol);
      CopMove16(cp, color[6], fgcol);
      CopMove16(cp, color[7], fgcol);

      CopWaitSafe(cp, Y(i), 0);
      linecol[i] = CopMove16(cp, color[0], 0x000);
      CopMove16(cp, color[9+k], 0x000);
      CopMove16(cp, color[10+k], 0x000);
      CopMove16(cp, color[11+k], 0x000);
      CopMove16(cp, bplcon1, NULL);
    }
  }

  return CopListFinish(cp);
}

static void Load(void) {
  TrackInit(&GlitchAnimFrame);
  TrackInit(&GlitchHPos);

  object = NewObject3D(&dna_helix);
  object->translate.z = fx4i(-256);
}

static void UnLoad(void) {
  DeleteObject3D(object);
}

static void Init(void) {
  TimeWarp(dna3d_start);

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_INTERLEAVED);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_INTERLEAVED);

  EnableDMA(DMAF_BLITTER | DMAF_BLITHOG);
  BitmapClearI(screen[0]);
  BitmapClearI(screen[1]);
  WaitBlitter();

  SetupDisplayWindow(MODE_LORES, X(32), Y(0), WIDTH, HEIGHT);
  SetupBitplaneFetch(MODE_LORES, X(32), WIDTH);
  SetupMode(MODE_DUALPF, DEPTH + necrocoq_depth);
  LoadColors(bobs_colors, 0);

  /* reverse playfield priorities */
  custom->bplcon2 = 0;
  /* AGA fix */
  custom->bplcon3 = BPLCON3_PF2OF0;

  /* bitplane modulos for both playfields */
  custom->bpl2mod = WIDTH / 8 * (necrocoq_depth - 1);
  custom->bpl1mod = WIDTH / 8 * (DEPTH - 1);

  cp = MakeCopperList();
  CopListActivate(cp);
  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  BlitterStop();
  CopperStop();

  DeleteCopList(cp);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
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
      short *p = &node[0].point.x;

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
      short *p = &node[1].point.x;

      /* (radius + helix_radius * cos(phi)) * cos(theta) */
      *p++ = swap16((short)(radius + cos_phi) * cos_theta);
      /* (radius + helix_radius * sin(phi)) * sin(theta) */
      *p++ = swap16((short)(radius + sin_phi) * sin_theta);
      /* helix_radius * sin(phi) */
      *p++ = sin_phi >> 3;
    }

    node += 2;
  }
}

#define BOBW 48
#define BOBH 32

typedef struct BobDesc {
  short offset;
  short x_align;
  short y_align;
  short bltsize;
} BobDescT;

#define BOBDESC(Y, H)                                           \
  (BobDescT){                                                   \
    .offset = (Y) * (BOBW / 8) * DEPTH,                         \
    .x_align = -16,                                             \
    .y_align = -(H) / 2,                                        \
    .bltsize = ((H) * DEPTH << 6) | (BOBW / 16),                \
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

      short bltshift, bltsize;
      void *apt, *dpt;

      z -= MINZ;
      z >>= 5;
      z &= ~7;

      {
        short *desc = (short *)(_bobdesc + z);

        apt = src + *desc++;
        x += *desc++;
        y += *desc++;
        bltsize = *desc++;
      }

      bltshift = rorw(x & 15, 4) | (SRCA | SRCB | DEST) | A_OR_B;

#if 1
      y <<= 5;
      y += y + y;     /* y *= 96 */
      y += (x & ~15) >> 3;
      dpt = dst + y;
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

static void SetBackgroundColor(short color) {
  CopInsT **lineptr = linecol;
  short i;

  for (i = 0; i < necrocoq_height; i++) {
    CopInsT *ins = *lineptr++;

    CopInsSet16(ins++, color);
    CopInsSet16(ins++, color);
    CopInsSet16(ins++, color);
    CopInsSet16(ins++, color);
  }
}

static void ChangeBackgroundColor(short n, short cols) {
  short p = envelope[n % envelope_length];
  u_short *data = necrochicken_cols[p];
  CopInsT **lineptr = linecol;
  short i;
  (void)necrochicken_cols;
  (void)necrochicken2_cols;

  if (cols == 0) {
    p = envelope2[n % (envelope_length - 8)];
    data = fadein_cols[p];
  } else if (cols == 1) {
    data = necrochicken_cols[p];
  } else if (cols == 2) {
    data = necrochicken2_cols[p];
  } else if (n % 4 >= 2) {
    data = necrochicken2_cols[p];
  }

  for (i = 0; i < necrocoq_height; i++) {
    CopInsT *ins = *lineptr++;

    CopInsSet16(ins++, *data++);
    CopInsSet16(ins++, *data++);
    CopInsSet16(ins++, *data++);
    CopInsSet16(ins++, *data++);
  }
}

static void ChangeBobsColor(short cols) {
  CopInsT **bobsptr = bobscol;
  u_short *data = bobs_cols_pixels;
  short i;

  if (cols == 2) {
    data = bobs_cols2_pixels;
  } else if (cols == 3 && (frameCount >> 1) % 4 >= 2) {
    data = bobs_cols2_pixels;
  }

  for (i = 0; i < necrocoq_height; i++) {
    CopInsT *ins = *bobsptr++;

    CopInsSet16(ins++, *data++);
    CopInsSet16(ins++, *data);
    CopInsSet16(ins++, *data++);
    CopInsSet16(ins++, *data);
    CopInsSet16(ins++, *data);
    CopInsSet16(ins++, *data);
    CopInsSet16(ins++, *data++);
  }
}

PROFILE(TransformObject);
PROFILE(DrawObject);

static void Render(void) {
  static __code bool show_cock = true;
  static __code bool new_bobs = false;
  static __code short mod = 0;
  static __code bool aux = true;
  static __code bool idx = 20;

  BitmapClearI(screen[active]);

  if (frameCount < dna3d_start + (dna3d_end / 6)) {
    SetBackgroundColor(0x000);
  } else if (show_cock) {
    ChangeBackgroundColor(idx, 0);
    ++idx;
    if (idx > 31) {
      show_cock = false;
    }
  }
 
  if (frameCount >  dna3d_start + (2 * (dna3d_end / 6)) &&
      frameCount <= dna3d_start + (3 * (dna3d_end / 5))){
    if (aux) {
      aux = false;
      mod = ((frameCount >> 1) % envelope_length);
    }
    ChangeBackgroundColor((frameCount >> 1) - mod, 1);
  }

  if (frameCount >  dna3d_start + (3 * (dna3d_end / 5)) &&
      frameCount <= dna3d_start + (4 * (dna3d_end / 6))) {
    new_bobs = true;
    ChangeBackgroundColor((frameCount >> 1) - mod, 3);
    ChangeBobsColor(3);
  }

  if (frameCount > dna3d_start + (4 * (dna3d_end / 6))) {
    new_bobs = true;
    ChangeBackgroundColor((frameCount >> 1) - mod, 2);
    ChangeBobsColor(2);
    (void)ChangeBobsColor;
  }

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
    if (new_bobs) {
      DrawFlares(object, bobs2.planes[0], screen[active]->planes[0], custom);
    } else {
      DrawFlares(object, bobs.planes[0], screen[active]->planes[0], custom);
    }
    DrawLinks(object, screen[active]->planes[1], custom);
  }
  ProfilerStop(DrawObject);

  TaskWaitVBlank();

  CopInsSet32(&bplptr[0], screen[active]->planes[0]);
  CopInsSet32(&bplptr[2], screen[active]->planes[1]);
  CopInsSet32(&bplptr[4], screen[active]->planes[2]);

  active ^= 1;
}

// For glitches
static void VBlank(void) {
  short i = 0;
  short animFrame = TrackValueGet(&GlitchAnimFrame, frameCount);
  short line = TrackValueGet(&GlitchHPos, frameCount);

  CopInsT *ins = linecol[0];

  // Please, let's make something other than 0 
  // a default track value >_<
  if (line != 0) {
    hPos = line;
  }

  for (i = 0; i < 31; i++) {
    ins = linecol[hPos+i];
    CopInsSet16(&ins[4], tears[animFrame][i]);
  }
}

EFFECT(Dna3D, Load, UnLoad, Init, Kill, Render, VBlank);
