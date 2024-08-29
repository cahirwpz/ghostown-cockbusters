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
static __code CopInsPairT *bplptr;
static CopListT *cp;
static __code short active = 0;
static __code short maybeSkipFrame = 0;
static __code SprDataT *sprdat;

#include "data/cock_scene_3.c"
#include "data/cock-pal.c"
#include "data/ksywka1.c"
#include "data/ksywka2.c"


static __code SpriteT *active_sprite;
static __code short *active_pal;
//#include "data/cock-techno.c"
/* Reading polygon data */
static short current_frame = 0;
static CopInsT *colors;
static CopInsPairT *sprmoves;


static CopListT *MakeCopperList(void) {
  CopListT *cp = NewCopList(100 + gradient.height * (gradient.width + 1) );
  CopInsPairT *sprptr = CopSetupSprites(cp);
  sprmoves = sprptr;
  bplptr = CopSetupBitplanes(cp, screen, DEPTH);
  {
    short *pixels = gradient.pixels;
    short i, j;
    
    for(i = 0; i < 8; i++){
      CopInsSetSprite(&sprptr[i], &active_sprite[i]);    
    }
    colors = CopSetColor(cp, 16, 0xdead);
    for(i = 1; i < 8; i++){
      CopSetColor(cp, i+16, 0xdead);    
    }
    
    if(1) for (i = 0; i < HEIGHT / 10; i++) {
      CopWait(cp, Y(YOFF + i * 10 - 1), 0xde);
      for (j = 0; j < 16; j++) CopSetColor(cp, j, *pixels++);
    }
   
  }
  return CopListFinish(cp);
}
  

static void Init(void) {
  //TimeWarp(cock_techno_start).
  //TrackInit(&spritepos);
  screen = NewBitmap(WIDTH, HEIGHT, DEPTH + 1, BM_CLEAR);
  EnableDMA(DMAF_BLITTER);
  BitmapClear(screen);
  WaitBlitter();

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(YOFF), WIDTH, HEIGHT);
  
  EnableDMA(DMAF_RASTER);

  //width 8 sprites, NSPRITE nicknames
  Log("sprdat = %p\n", &sprdat);
  Log("ks1 = %p\n", ks1);

  //memset(sprdat, 0x55, SprDataSize(32,2) * 8 * NSPRITE);
  //*((short*) &sprdat[1]) = 0x1111;
  
  
  //memcpy(sprdat, ks1_pixels, SprDataSize(32,2));

  LoadColors(ks1_pal_colors, 16);
  LoadColors(ks2_pal_colors, 20);
  LoadColors(ks2_pal_colors, 24);
  LoadColors(ks2_pal_colors, 28);
  
  (void) ks2_pal_colors; // klappe halten
  (void) ks2;
  active_sprite = ks1;
  active_pal = ks2_pal_colors;
  cp = MakeCopperList();
  CopListActivate(cp);

  //memset(screen, 0xa000, 0x55);
  
  EnableDMA(DMAF_SPRITE | DMAF_RASTER);

}

static void Kill(void) {
  BlitterStop();
  CopperStop();
  DeleteCopList(cp);

  DeleteBitmap(screen);
  MemFree(sprdat);
  DisableDMA(DMAF_SPRITE);

}

static inline void DrawEdge(short *coords, void *dst,
                            CustomPtrT custom_ asm("a6")) {
  static __chip short tmp;

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
  custom_->bltdpt = &tmp;
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
  active_sprite = ((frameCount >> 6) & 0x01) ? ks1 : ks2;
  active_pal = ((frameCount >> 6) & 0x01) ? ks1_pal_colors : ks2_pal_colors;
  Log("active_sprite = %p\n", active_sprite);
  // overwrite ~~active~~ copperlist positions to move sprites
  CopInsSet32(&sprmoves[0], active_sprite[0].sprdat);
  CopInsSet32(&sprmoves[1], active_sprite[1].sprdat);
  CopInsSet32((CopInsPairT*) &cp->entry[4], active_sprite[2].sprdat);
  CopInsSet32((CopInsPairT*) &cp->entry[6], active_sprite[3].sprdat);

  CopInsSet16( &colors[0], active_pal[0]);
  CopInsSet16( &colors[1], active_pal[1]);
  CopInsSet16( &colors[2], active_pal[2]);
  CopInsSet16( &colors[3], active_pal[3]);

  //CopInsSet16( &cp->entry[9], active_pal[1]);
  //CopInsSet16( &cp->entry[10], active_pal[2]);
  //CopInsSet16( &cp->entry[11], active_pal[3]);
  
  //CopInsSet32((CopInsPairT*) &cp->entry[2], active_sprite[1].sprdat);
  //CopInsSet32((CopInsPairT*) &cp->entry[4], active_sprite[2].sprdat);
  //CopInsSet32((CopInsPairT*) &cp->entry[6], active_sprite[3].sprdat);
  
  /* Frame lock the effect to 25 FPS */
  if (maybeSkipFrame) {
    maybeSkipFrame = 0;
    if (frameCount - lastFrameCount == 1) {
      TaskWaitVBlank();
      return;
    }
  }

  SpriteUpdatePos(&ks1[0], X(0x60), Y(40));
  SpriteUpdatePos(&ks1[1], X(0x70), Y(40) - (frameCount & 0x1F));
  SpriteUpdatePos(&ks1[2], X(0x80), Y(40));
  SpriteUpdatePos(&ks1[3], X(0x90), Y(40));
  SpriteUpdatePos(&ks2[0], X(0x60), Y(150));
  SpriteUpdatePos(&ks2[1], X(0x70), Y(150));
  SpriteUpdatePos(&ks2[2], X(0x80), Y(150));
  SpriteUpdatePos(&ks2[3], X(0x90), Y(150));
  

  
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
      CopInsSet32(&bplptr[n], screen->planes[i]);
    }
  }
  }
  TaskWaitVBlank();
 
  active = mod16(active + 1, DEPTH + 1);
  maybeSkipFrame = 1;
}

EFFECT(CockTechno, NULL, NULL, Init, Kill, Render, NULL);
