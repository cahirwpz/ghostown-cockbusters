#include <2d.h>
#include <blitter.h>
#include <copper.h>
#include <effect.h>
#include <fx.h>
#include <line.h>
#include <pixmap.h>
#include <types.h>
#include <sprite.h>
#include <sync.h>
#include <palette.h>
#include <system/memory.h>

#define WIDTH 320
#define HEIGHT 180
#define YOFF ((256 - HEIGHT) / 2)
#define DEPTH 4
#define NSPRITE 3

//static __code SpriteT sprite[8 * NSPRITE];
static __code BitmapT *screen;
static __code CopInsPairT *bplptr[2];
static CopListT *cp[2];
static __code short active = 0;
static __code short maybeSkipFrame = 0;
static __code SprDataT *sprdat;
static __code short activecl = 0;

#include "data/cock_scene_3.c"
#include "data/cock-pal.c"
#include "data/cock-pal2.c"
#include "data/cock-pal3.c"
#include "data/cock-pal4.c"
#include "data/cock-pal5.c"
#include "data/cock-pal6.c"
#include "data/cock-pal7.c"
#include "data/cock-pal8.c"
#include "data/cock-pal9.c"
#include "data/cock-pal10.c"
#include "data/cock-pal11.c"

#include "data/cock-techno.c"

#include "data/cahir.c"
#include "data/codi.c"
#include "data/jazzcat.c"
#include "data/polprog.c"
#include "data/slayer.c"
#include "data/slizgi.c"
#include "data/spook.c"
#include "data/yumi.c"
#include "data/zuko.c"


static const PixmapT *palettes[] = {
  &gradient11, &gradient10, &gradient9,
  &gradient8,  &gradient7,  &gradient6,
  &gradient5,  &gradient4,  &gradient3,
  &gradient2,  &gradient1
};

static SpriteT *halloffame[] = {
  cahir, slayer, jazzcat, slizgi, codi,
  spook, zuko,   yumi,    polprog
};


static __code SpriteT *active_sprite;
static __code short *active_pal;
/* Reading polygon data */
static short current_frame = 0;
static CopInsT *colors[2];
static CopInsPairT *sprmoves[2];
static __code short oldgradientno = 10;
static __code short oldspriteno   = 0;


static CopListT *MakeCopperList(CopListT *cp, short gno, short act) {

  CopInsPairT *sprptr = CopSetupSprites(cp);
  sprmoves[act] = sprptr;
  bplptr[act] = CopSetupBitplanes(cp, screen, DEPTH);
  {
    short *pixels = palettes[gno]->pixels;

    short i, j;
    
    for(i = 0; i < 8; i++){
      CopInsSetSprite(&sprptr[i], &active_sprite[i]);    
    }
    colors[act] = CopSetColor(cp, 16, 0xdead);
    for(i = 1; i < 16; i++){
      CopSetColor(cp, i+16, 0xdead);    
    }

    for (i = 0; i < HEIGHT / 10; i++) {
      u_short c;
      u_short k, n;

      CopWait(cp, Y(YOFF + i * 10 - 1), 0xde);

      CopSetColor(cp, 0, 0);
      for (j = 1, n = 1; j < 16; n += n) {
        c = *pixels++;
        for (k = 0; k < n; k++, j++)
          CopSetColor(cp, j, c);
      }
    }
   
  }
  return CopListFinish(cp);
}
  

static void Init(void) {
  TimeWarp(cock_techno_start);
  TrackInit(&spritepos);
  TrackInit(&spriteno);
  TrackInit(&techno_gradientno);


  screen = NewBitmap(WIDTH, HEIGHT, DEPTH + 1, BM_CLEAR);
  EnableDMA(DMAF_BLITTER);
  BitmapClear(screen);
  WaitBlitter();

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(YOFF), WIDTH, HEIGHT);
  
  EnableDMA(DMAF_RASTER);


  active_sprite = cahir;
  active_pal = cahir_colors;
  cp[0] = NewCopList(100 + gradient1.height * ((1 << DEPTH) + 1));
  MakeCopperList(cp[0], 0, 0);
  cp[1] = NewCopList(100 + gradient1.height * ((1 << DEPTH) + 1));
  MakeCopperList(cp[1], 0, 1);

  CopListActivate(cp[0]);

  //memset(screen, 0xa000, 0x55);
  
  EnableDMA(DMAF_SPRITE | DMAF_RASTER);

}

static void Kill(void) {
  BlitterStop();
  CopperStop();
  DeleteCopList(cp[0]);

  DeleteBitmap(screen);
  MemFree(sprdat);
  DisableDMA(DMAF_SPRITE);

}

static inline void DrawEdge(short *coords, void *dst,
                            CustomPtrT custom_ asm("a6")) {
  /* XXX awful hack to find some unused chip memory to write first pixel to!
   * This avoids creating small memory allocation. */
  short *tmp = (short *)cp[activecl]->curr;

  short bltcon0, bltcon1, bltsize, bltbmod, bltamod;
  short dmin, dmax, derr, offset;
  int bltapt;

  short x0 = *coords++;
  short y0 = *coords++;
  short x1 = *coords++;
  short y1 = *coords++;

  if (y0 == y1)
    return;

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

  offset = ((y0 << 5) + (y0 << 3) + (x0 >> 3)) & ~1;

  bltcon0 = rorw(x0 & 15, 4) | BC0F_LINE_EOR;
  bltcon1 |= rorw(x0 & 15, 4);

  dmin <<= 1;
  derr = dmin - dmax;
  if (derr < 0)
    bltcon1 |= SIGNFLAG;

  bltamod = derr - dmax;
  bltbmod = dmin;
  bltsize = (dmax << 6) + 66;
  bltapt = derr;

  _WaitBlitter(custom_);

  custom_->bltcon0 = bltcon0;
  custom_->bltcon1 = bltcon1;
  custom_->bltapt = (void *)bltapt;
  custom_->bltbmod = bltbmod;
  custom_->bltamod = bltamod;
  custom_->bltcpt = dst + offset;
  custom_->bltdpt = tmp;
  custom_->bltsize = bltsize;
}

/* Get (x,y) on screen position from linear memory repr */
#define CalculateXY(data, x, y)         \
  asm("clrl  %0\n\t"                    \
      "movew %2@+,%0\n\t"               \
      "divu  %3,%0\n\t"                 \
      "movew %0,%1\n\t"                 \
      "swap  %0"                        \
      : "=d" (x), "=r" (y), "+a" (data) \
      : "i" (WIDTH));

static short *PickFrame(void) {
  short num = current_frame++;

  for (;;) {
    if (num < cock_scene_3_frames)
      return cock_scene_3_frame[num];
    num -= cock_scene_3_frames;
  }
}

static void DrawFrame(void *dst, CustomPtrT custom_ asm("a6")) {
  static __code Point2D points[256];
  short *data = PickFrame();
  short n;

  _WaitBlitter(custom_);

  /* prepare for line drawing */
  custom_->bltafwm = 0xffff;
  custom_->bltalwm = 0xffff;
  custom_->bltbdat = 0xffff;
  custom_->bltadat = 0x8000;
  custom_->bltcmod = WIDTH / 8;
  custom_->bltdmod = WIDTH / 8;

  while ((n = *data++)) {
    {
      short *point = (short *)points;
      short k = n;
      while (--k >= 0) {
        short x = 0, y = 0;
        CalculateXY(data, x, y);
        *point++ = x;
        *point++ = y;
      }
      *point++ = points[0].x;
      *point++ = points[0].y;
    }

    {
      Point2D *point = points;
      while (--n >= 0)
        DrawEdge((short *)point++, dst, custom_);
    }
  }
}

PROFILE(AnimRender);

static void Render(void) {
  //short actcl = 0;

  /* Frame lock the effect to 25 FPS */
  if (maybeSkipFrame) {
    maybeSkipFrame = 0;
    if (frameCount - lastFrameCount == 1) {
      TaskWaitVBlank();
      return;
    }
  }

  
  if(1) {   
  ProfilerStart(AnimRender);
  {
    BlitterClear(screen, active);
    DrawFrame(screen->planes[active], custom);
    BlitterFill(screen, active);
  }
  ProfilerStop(AnimRender);

  // chicken afterglow
  if(1) {
    short n = DEPTH;

    while (--n >= 0) {
      short i = mod16(active + n + 1 - DEPTH, DEPTH + 1);
      if (i < 0) i += DEPTH + 1;
      CopInsSet32(&bplptr[activecl][n], screen->planes[i]);
    }
  }
  }
  TaskWaitVBlank();
  
  
  
  active = mod16(active + 1, DEPTH + 1);
  maybeSkipFrame = 1;
}

static void VBlankHandler(void){
  //short actcl = 0;

  // overwrite active copperlist positions to move sprites
  // would be nice to have this in the same place as MakeCopperList but its
  // not 1:1
  {
    short i = 0;
    for(i = 0; i < 16; i++){
      CopInsSet16( &colors[activecl][i], active_pal[i&3]);
    }
    for(i = 0; i < 8; i++){
      CopInsSet32(&sprmoves[activecl][i], active_sprite[i].sprdat);
      SpriteUpdatePos(&active_sprite[i], X(0x10 + i*0x10), Y(TrackValueGet(&spritepos, frameCount)));
    }
  }
  

  // Swap copperlist and change palette
  {
    u_short gno = TrackValueGet(&techno_gradientno, frameCount);
    if(oldgradientno != gno) {
      oldgradientno = gno;
      activecl ^= 1;
      // this is not a leak as fas as I can tell
      // since the length is not changed. We just need
      // to regenerate the list starting w/ the first instruction
      Log("CL swap prepare. gno=%d activecl=%d\n", gno, activecl);

      cp[activecl]->curr = cp[activecl]->entry;
      MakeCopperList(cp[activecl], gno, activecl);
      
      Log("CL swap done gno=%d activecl=%d\n", gno, activecl);
    }
  }
  custom->cop1lc = (u_int)cp[activecl]->entry;

  // Change active sprite
  {
    u_short sno = TrackValueGet(&spriteno, frameCount);
    if(oldspriteno != sno){
      oldspriteno = sno;
      active_sprite = halloffame[sno];
    }
  }

}


EFFECT(CockTechno, NULL, NULL, Init, Kill, Render, VBlankHandler);
