#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <fx.h>
#include <pixmap.h>
#include <sprite.h>
#include <system/memory.h>
#include <c2p_1x1_4.h>

#define S_WIDTH 320
#define S_HEIGHT 256
#define S_DEPTH 4

#define WIDTH 64
#define HEIGHT 64

static PixmapT *segment_p;
static BitmapT *segment_bp;

#define Log(...)

// target screen bitplanes
static BitmapT dragon_bp;
static __data_chip char dragon_bitplanes[S_WIDTH * S_HEIGHT * S_DEPTH / 8];

static PixmapT *dragon_chip;

static u_char *texture_hi;
static u_char *texture_lo;

static BitmapT *screen;
static SprDataT *sprdat;
static SpriteT sprite[8];

#include "data/dragon-bg.c"
#include "data/ball.c"

static short active = 0;
static CopListT *cp;

#define UVMapRenderSize (WIDTH * HEIGHT / 2 * 10 + 2)
void (*UVMapRender)(u_char *chunky asm("a0"),
                    u_char *textureHi asm("a1"),
                    u_char *textureLo asm("a2"));


static void CropPixmap(const PixmapT *input, u_short x0, u_short y0,
		       short width, short height, PixmapT* output);

#if 0
static void PixmapToTexture(const PixmapT *image,
                            PixmapT *imageHi, PixmapT *imageLo)
{
  u_char *data = image->pixels;
  int size = image->width * image->height;
  /* Extra halves for cheap texture motion. */
  u_short *hi0 = imageHi->pixels;
  u_short *hi1 = imageHi->pixels + size;
  u_short *lo0 = imageLo->pixels;
  u_short *lo1 = imageLo->pixels + size;
  short n = size / 2;

  while (--n >= 0) {
    u_char a = *data++;
    u_short b = ((a << 8) | (a << 1)) & 0xAAAA;
    /* [a0 b0 a1 b1 a2 b2 a3 b3] => [a0 -- a2 -- a1 -- a3 --] */
    *hi0++ = b;
    *hi1++ = b;
    /* [a0 b0 a1 b1 a2 b2 a3 b3] => [-- b0 -- b2 -- b1 -- b3] */
    *lo0++ = b >> 1;
    *lo1++ = b >> 1;
  }
}
#endif
static void MakeUVMapRenderCode(void) {
  u_short *code = (void *)UVMapRender;
  u_short *data = uvmap;
  short n = WIDTH * HEIGHT / 2;
  short uv;

  while (n--) {
    if ((uv = *data++) >= 0) {
      *code++ = 0x1029;  /* 1029 xxxx | move.b xxxx(a1),d0 */
      *code++ = uv;
    } else {
      *code++ = 0x7000;  /* 7000      | moveq  #0,d0 */
    }
     if ((uv = *data++) >= 0) {
      *code++ = 0x802a;  /* 802a yyyy | or.b   yyyy(a2),d0 */
      *code++ = uv;
    }
    *code++ = 0x10c0;    /* 10c0      | move.b d0,(a0)+    */
  }

  *code++ = 0x4e75; /* rts */
}


static CopListT *MakeCopperList(int active) {
  CopListT *cp = NewCopList(80);
  CopInsPairT *sprptr = CopSetupSprites(cp);
  short i;
  (int*) active = 0;
  CopSetupBitplanes(cp, screen, S_DEPTH);
  for (i = 0; i < 8; i++)
    CopInsSetSprite(&sprptr[i], &sprite[i]);
  return CopListFinish(cp);
}


static void Init(void) {
  u_short bitplanesz = ((dragon_width + 15) & ~15) / 8 * dragon_height;

  //segment_bp and segment_p are bitmap and pixmap for the magnified segment
  //ELF->ST: BM_CPUONLY was BM_DISPLAYABLE
  segment_bp = NewBitmap(WIDTH, HEIGHT, S_DEPTH, BM_CLEAR);
  segment_p = NewPixmap(WIDTH, HEIGHT, PM_CMAP4, MEMF_CHIP);
  screen = NewBitmap(S_WIDTH, S_HEIGHT, S_DEPTH, BM_CLEAR);

  texture_hi = MemAlloc(WIDTH*HEIGHT, MEMF_CHIP);
  texture_lo = MemAlloc(WIDTH*HEIGHT, MEMF_CHIP);


  dragon_chip = NewPixmap(dragon_width, dragon_height, PM_CMAP4, MEMF_CHIP);

  // c2p requires target bitplanes to be contiguous in memory, therefore we
  // allocate the BitmapT manually

  dragon_bp.width  = dragon_width;
  dragon_bp.height = dragon_height;
  dragon_bp.bytesPerRow = ((dragon_width + 15) & ~15) / 8;
  dragon_bp.bplSize = dragon_bp.bytesPerRow * dragon_bp.height;
  dragon_bp.depth = 4;
  dragon_bp.flags = BM_STATIC;
  dragon_bp.planes[0] = dragon_bitplanes;
  dragon_bp.planes[1] = dragon_bp.planes[0] + dragon_bp.bplSize;
  dragon_bp.planes[2] = dragon_bp.planes[1] + dragon_bp.bplSize;
  dragon_bp.planes[3] = dragon_bp.planes[2] + dragon_bp.bplSize;

  Log("sizeof(dragon_pixels) =  $%lx\n", sizeof(dragon_pixels));
  Log("sizeof bitplane =  $%x * depth $%x = total $%lx\n",
      dragon_bp.bplSize, dragon_bp.depth,
      ((long int) dragon_bp.bplSize) * dragon_bp.depth);
  Log("&(dragon_bp->planes[0]) =  $%p\n", dragon_bp.planes[0]);
  Log("dragon_height =  $%x = %d\n", dragon_height, dragon_height);
  Log("dragon_width  =  $%x = %d\n", dragon_width, dragon_width);

  EnableDMA(DMAF_BLITTER | DMAF_BLITHOG);
  Log("&dragon_bp = %p\n", dragon_bp);



#if 1
  {
    void *tmp;
    tmp = dragon_chip->pixels;
    memcpy(dragon_chip, &dragon, sizeof(dragon));
    dragon_chip->pixels = tmp;
    memcpy(dragon_chip->pixels, dragon.pixels, dragon_width * dragon_height / 2);
    Log("dragon_chip->pixels = %p\n", dragon_chip->pixels);
  }
#endif

  // Warning: c2p works in place. dragon_chip is destroyed.
  ChunkyToPlanar(dragon_chip, &dragon_bp);
  Log("c2p done");
  // We have to use blitter to crop the pixmap in Render(),
  //so dragon needs to stay in chipmem.
#if 1
  {
    void *tmp;
    tmp = dragon_chip->pixels;
    memcpy(dragon_chip, &dragon, sizeof(dragon));
    dragon_chip->pixels = tmp;
    memcpy(dragon_chip->pixels, dragon.pixels, dragon_width * dragon_height / 2);
    Log("dragon_chip->pixels = %p\n", dragon_chip->pixels);
  }
#endif



#if 1
  //Copy dragon bitmap to background
  memcpy(screen->planes[0], dragon_bp.planes[0],
         S_WIDTH * S_HEIGHT * S_DEPTH / 8);
#endif

#if 1
  UVMapRender = MemAlloc(UVMapRenderSize, MEMF_PUBLIC);
  MakeUVMapRenderCode();
#endif

  Log("dragon_chip->pixels = %p\n", dragon_chip->pixels);

  sprdat = MemAlloc(SprDataSize(64, 2) * 8 * 2, MEMF_CHIP | MEMF_CLEAR);

  {
    SprDataT *dat = sprdat;
    short j; //i, j;

    //for (i = 0; i < 2; i++)
    for (j = 0; j < 8; j++) {
      MakeSprite(&dat, 64, j & 1, &sprite[j]);
      EndSprite(&dat);
    }
  }

  SetupPlayfield(MODE_LORES, S_DEPTH, X(0), Y(0), S_WIDTH, S_HEIGHT);
  LoadColors(dragon_pal_colors, 0);
  LoadColors(dragon_pal_colors, 16);

  cp = MakeCopperList(0);
  //CopListActivate(cp);


  EnableDMA(DMAF_RASTER | DMAF_SPRITE);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_BLITTER | DMAF_SPRITE);

  DeleteCopList(cp);
  //DeletePixmap(textureHi);
  //DeletePixmap(textureLo);
  MemFree(UVMapRender);
  MemFree(sprdat);

  DeletePixmap(segment_p);
  DeleteBitmap(segment_bp);
}


#if 1
#define C2P_MASK0 0x00FF
#define C2P_MASK1 0x0F0F
#define C2P_MASK2 0x3333
#define C2P_MASK3 0x5555
// BLTSIZE register value
#define BLTSIZE_VAL(w, h) (h << 6 | (w))
// Blit area given these WIDTH, HEIGHT, MODULO settings
#define BLTAREA(w, h, mod) (h == 0 ? ((2*w + mod) * 1024) : ((2*w + mod) * h))
// Calculate necesary height for the area
// The macro will never return more than 1024h
#define _BLITHEIGHT(w, area, mod) ((area) / (2*w + mod))
#define BLITHEIGHT(w, area, mod) (_BLITHEIGHT(w, area, mod) >= 1024 ? 0 : _BLITHEIGHT(w, area, mod))

/* C2P pass 1 is AC + BNC, pass 2 is ANC + BC */
#define C2P_LF_PASS1 (NABNC | ANBC | ABNC | ABC)
#define C2P_LF_PASS2 (NABC | ANBNC | ABNC | ABC)

/*
 * Chunky to Planar conversion using blitter
 */
static void ChunkyToPlanar(PixmapT *input, BitmapT *output) {
  char *planes = output->planes[0];
  char *chunky = input->pixels;

  u_short blsz = 0;
  u_short chunkysz = input->width * input->height / 2;
  u_short planesz  = output->bplSize;
  u_short planarsz = planesz * 4;
  u_short blith    = 0;
  u_short blitarea = 0;
  u_short i = 0;

  Log("ChunkyToPlanar: input  = %p  output = %p\n", input, output);
  Log("ChunkyToPlanar: chunky = %p  planes = %p\n", chunky, planes);
  Log("ChunkyToPlanar: inputh = %d\n", input->height);
  Log("ChunkyToPlanar: inputw = %d\n", input->width);

  Log("ChunkyToPlanar: 1 bitplane size = 0x%x\n", planesz );
  Log("ChunkyToPlanar: planar size = 0x%x\n", planesz * 4);

  Log("ChunkyToPlanar: blit height   = 0x%x\n", blith );


  // This is implemented as in prototypes/c2p/c2p_1x1_4bp_blitter_backforth.py
  // rem: modulos and pointers are in bytes

  //TODO: Specialcase last non-1024h blit
  //TODO: Parametrize the blit sizes for screen size other than full screen

  while(blitarea < planarsz){
    // Swap 8x4, pass 1
    {
      blith = BLITHEIGHT(2, planarsz - blitarea, 4);
      Log("8x4: Blitheight = %x\n", blith);

      WaitBlitter();

      /* ((a >> 8) & 0x00FF) | (b & ~0x00FF) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS1 | ASHIFT(8);
      custom->bltcon1 = 0;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 4;
      custom->bltbmod = 4;
      custom->bltdmod = 4;
      custom->bltcdat = C2P_MASK0;

      custom->bltapt = chunky + 4 + blitarea;
      custom->bltbpt = chunky     + blitarea;
      custom->bltdpt = planes     + blitarea;
      custom->bltsize = BLTSIZE_VAL(2, blith);
    }

    // Swap 8x4, pass 2
    {
      WaitBlitter();

      /* ((a << 8) & ~0x00FF) | (b & 0x00FF) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS2 | ASHIFT(8);
      custom->bltcon1 = BLITREVERSE;

      custom->bltapt = chunky - 6 + BLTAREA(2, blith, 4) + blitarea;
      custom->bltbpt = chunky - 2 + BLTAREA(2, blith, 4) + blitarea;
      custom->bltdpt = planes - 2 + BLTAREA(2, blith, 4) + blitarea;
      custom->bltsize = BLTSIZE_VAL(2, blith);
    }
    blitarea += BLTAREA(2, blith, 4);
    Log("8x4: Blitarea = %x\n", blitarea);
  }



  blitarea = 0;
  while(blitarea < planarsz){
     blith = BLITHEIGHT(1, planarsz - blitarea, 2);
     Log("4x2: Blitheight = %x\n", blith);

    // Swap 4x2, pass 1
    {
      WaitBlitter();

      /* ((a >> 4) & 0x0F0F) | (b & ~0x0F0F) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS1 | ASHIFT(4);
      custom->bltcon1 = 0;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 2;
      custom->bltbmod = 2;
      custom->bltdmod = 2;
      custom->bltcdat = C2P_MASK1;

      custom->bltapt = planes + 2 + blitarea;
      custom->bltbpt = planes     + blitarea;
      custom->bltdpt = chunky     + blitarea;
      custom->bltsize = BLTSIZE_VAL(1, blith);
    }

    // Swap 4x2, pass 2
    {
      WaitBlitter();

      /* ((a << 4) & ~0x0F0F) | (b & 0x0F0F) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS2 | ASHIFT(4);
      custom->bltcon1 = BLITREVERSE;

      custom->bltapt = planes + BLTAREA(1, blith, 2) - 4 + blitarea;
      custom->bltbpt = planes + BLTAREA(1, blith, 2) - 2 + blitarea;
      custom->bltdpt = chunky + BLTAREA(1, blith, 2) - 2 + blitarea;
      custom->bltsize = BLTSIZE_VAL(1, blith);
    }

    blitarea += BLTAREA(1, blith, 2);
    Log("4x2: Blitarea = %x\n", blitarea);
  }


  blitarea = 0;
  while(blitarea < planarsz){
    blith = BLITHEIGHT(2, planarsz - blitarea, 4);
    Log("2x2: Blitheight = %x\n", blith);

    // Swap 2x2, pass 1
    {
      WaitBlitter();

      /* ((a >> 2) & 0x3333) | (b & ~0x3333) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS1 | ASHIFT(2);
      custom->bltcon1 = 0;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 4;
      custom->bltbmod = 4;
      custom->bltdmod = 4;
      custom->bltcdat = C2P_MASK2;

      custom->bltapt = chunky + 4 + blitarea;
      custom->bltbpt = chunky     + blitarea;
      custom->bltdpt = planes     + blitarea;
      custom->bltsize = BLTSIZE_VAL(2, blith);
    }

    // Swap 2x2, pass 2
    {
      WaitBlitter();

      /* ((a << 2) & ~0x3333) | (b & 0x3333) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS2 | ASHIFT(2);
      custom->bltcon1 = BLITREVERSE;
      //custom->bltadat = 0xFFFF;
      //custom->bltbdat = 0x0000;

      custom->bltapt = chunky - 6 + BLTAREA(2, blith, 4) + blitarea;
      custom->bltbpt = chunky - 2 + BLTAREA(2, blith, 4) + blitarea;
      custom->bltdpt = planes - 2 + BLTAREA(2, blith, 4) + blitarea;
      custom->bltsize = BLTSIZE_VAL(2, blith);
    }

    blitarea += BLTAREA(2, blith, 4);
    Log("2x2: Blitarea = %x\n", blitarea);
  }

  // Copy to bitplanes - last swap
  // Numbers in quotes refer to bitplane numbers on test card.

  blitarea = 0;
  while(blitarea < planarsz){
    blith = BLITHEIGHT(1, planarsz - blitarea, 6);
    Log("bplcpy: Blitheight = %x\n", blith);

#if 1
    //Copy to bitplane 0, "4"
    {
      WaitBlitter();

      /* ((a >> 1) & 0x5555) | (b & ~0x5555) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS1 | ASHIFT(1);
      custom->bltcon1 = 0;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 6;
      custom->bltbmod = 6;
      custom->bltdmod = 0;
      custom->bltcdat = C2P_MASK3;

      custom->bltapt = planes + 2 + blitarea;
      custom->bltbpt = planes     + blitarea;
      custom->bltdpt = chunky + 3*planesz + i*BLTAREA(1, blith, 0);
      custom->bltsize = BLTSIZE_VAL(1, blith);
    }
    WaitBlitter();
#endif

#if 1
    //Copy to bitplane 1, "3"
    {
      WaitBlitter();

      /* ((a << 1) & ~0x5555) | (b & 0x5555) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS2 | ASHIFT(1);
      //custom->bltcon0 = DEST | A_TO_D;
      custom->bltcon1 = BLITREVERSE;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 6;
      custom->bltbmod = 6;
      custom->bltdmod = 0;
      custom->bltcdat = C2P_MASK3;

      custom->bltapt = planes + BLTAREA(1, blith, 6) - 8 + blitarea;
      custom->bltbpt = planes + BLTAREA(1, blith, 6) - 6 + blitarea;
      custom->bltdpt = chunky + 2*planesz - 2 + (i+1)*BLTAREA(1, blith, 0);

      custom->bltsize = BLTSIZE_VAL(1, blith);
    }
    WaitBlitter();

#endif

#if 1
    //Copy to bitplane 2, "2"
    {
      WaitBlitter();

      /* ((a >> 1) & 0x5555) | (b & ~0x5555) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS1 | ASHIFT(1);
      custom->bltcon1 = 0;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 6;
      custom->bltbmod = 6;
      custom->bltdmod = 0;
      custom->bltcdat = C2P_MASK3;

      custom->bltapt = planes + 6 + blitarea;
      custom->bltbpt = planes + 4 + blitarea;
      custom->bltdpt = chunky + planesz + i*BLTAREA(1, blith, 0);
      custom->bltsize = BLTSIZE_VAL(1, blith);
    }
    WaitBlitter();
#endif

#if 1
    //Copy to bitplane 3, "1"
    {
      WaitBlitter();

      /* ((a << 1) & ~0x5555) | (b & 0x5555) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS2 | ASHIFT(1);
      custom->bltcon1 = BLITREVERSE;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 6;
      custom->bltbmod = 6;
      custom->bltdmod = 0;
      custom->bltcdat = C2P_MASK3;

      custom->bltapt = planes + BLTAREA(1, blith, 6) - 4 + blitarea;
      custom->bltbpt = planes + BLTAREA(1, blith, 6) - 2 + blitarea;
      custom->bltdpt = chunky - 2 + (i+1)*BLTAREA(1, blith, 0);
      custom->bltsize = BLTSIZE_VAL(1, blith);
    }
    WaitBlitter();

#endif
    blitarea += BLTAREA(1, blith, 6);
    Log("bplcpy: Blitarea AB = %x, blitarea D = %x\n",
	blitarea, i * BLTAREA(1, blith, 0));
    i++;

  }

  //memset(planes, 0, 0xa000);
  //Because there is an even number of steps here, c2p finishes with planar data
  //in chunky. These last 4 blits move it to planar, where it can be copied to
  //screen.

  //This is where it's getting complicated. We need to copy an arbitrary number of bytes
  //from chunky to planes.
  //The blith algorhitm is simple - modulo is zero
  //To copy the arbitrary number of bytes we should find the optimal blitter settings
  //But for now W = 32 seems to work for most cases...
  blith = BLITHEIGHT(32, planesz, 0);

  Log("chunky->planar copy blith = %x\n", blith);
  Log("chunky->planar copy area  = %x\n", BLTAREA(32, blith, 0));


#if 1
  // Copy bpl0 "1"
  {
    WaitBlitter();
    custom->bltcon0 = (SRCA| DEST) | A_TO_D;
    custom->bltcon1 = 0;
    custom->bltamod = 0;
    custom->bltdmod = 0;
    custom->bltapt = chunky;
    custom->bltdpt = planes;
    custom->bltsize = BLTSIZE_VAL(32, blith);
  }
#endif
#if 1
  // Copy bpl1 "2"
  {
    WaitBlitter();
    custom->bltcon0 = (SRCA | DEST) | A_TO_D;
    custom->bltcon1 = 0;
    custom->bltamod = 0;
    custom->bltdmod = 0;
    custom->bltapt = chunky + planesz;
    custom->bltdpt = planes + planesz;
    custom->bltsize = BLTSIZE_VAL(32, blith);
  }
#endif
#if 1
  // Copy bpl2 "3"
  {
    WaitBlitter();
    custom->bltcon0 = (SRCA | DEST) | A_TO_D;
    custom->bltcon1 = 0;
    custom->bltamod = 0;
    custom->bltdmod = 0;
    //custom->bltadat = 0xF00;
    custom->bltapt = chunky + 2*planesz;
    custom->bltdpt = planes + 2*planesz;
    custom->bltsize = BLTSIZE_VAL(32, blith);
  }
#endif
#if 1
  // Copy bpl3 "4"
  {
    WaitBlitter();
    custom->bltcon0 = (SRCA | DEST) | A_TO_D;
    custom->bltcon1 = 0;
    custom->bltamod = 0;
    custom->bltdmod = 0;
    custom->bltapt = chunky + 3*planesz;
    custom->bltdpt = planes + 3*planesz;
    custom->bltsize = BLTSIZE_VAL(32, blith);
  }
#endif
  return;

}
#endif

static void PositionSprite(SpriteT sprite[8], short xo, short yo) {
  short x = X((S_WIDTH - WIDTH) / 2) + xo;
  short y = Y((S_HEIGHT - HEIGHT) / 2) + yo;
  short n = 4;

  while (--n >= 0) {
    SpriteT *spr0 = sprite++;
    SpriteT *spr1 = sprite++;

    SpriteUpdatePos(spr0, x, y);
    SpriteUpdatePos(spr1, x, y);

    x += 16;
  }
}


static void CropPixmap(const PixmapT *input, u_short x0, u_short y0,
		       short width, short height, PixmapT* output){
  //XXX: do this with blitter,

  u_short j = 0;
  u_short k = 0;
  for(j = y0; j < height+y0; j++){
    //We are all 4bpp now, so 2px per b
    memcpy(output->pixels + k*output->width/2,
	   input->pixels + j*input->width/2 + x0/2,
	   width/2);
    k++;
  }
}


#define POS4BPP(pixels, iw, x0, y0) ((pixels) + (y0)*(iw)/2 + (x0)/2)
static void CropPixmapBlitter(const PixmapT *input, u_short x0, u_short y0,
			      short width, short height, u_char* thi, u_char *tlo){

  // The blitter operated on words, therefore we have 4 different shift cases
  // depending how the edge of cropped pixmap is aligned to word boundaries.
  // That is, if x0 % 4 equals...
  // 0: no shift,        1: left shift by 4
  // 2: left shift by 8, 3: left shift by 12
  // TODO: implement <<8 and <<12 blocks


  short shiftamt = (x0 & 0x3) * 4;
  short lwm = 0x000f;
  u_char *chunky = input->pixels;

  {
    WaitBlitter();
    /* (A << 4) & 0xF0F0  */
    custom->bltcon0 = (SRCA | DEST) | ANBC | ABC | ASHIFT(shiftamt);
    custom->bltcon1 = BLITREVERSE;
    custom->bltafwm = -1;
    custom->bltalwm = lwm;
    custom->bltamod = (input->width - width) / 2;
    custom->bltdmod = 0;
    custom->bltcdat = 0xf0f0;
    custom->bltadat = 0;

    custom->bltapt = POS4BPP(chunky, input->width, x0+width, y0+width -1) - 2;
    custom->bltdpt = tlo + (width*height/2) - 2; //ok
    //BLTSIZE_VAL is in words. 4px per word
    custom->bltsize = BLTSIZE_VAL(width/4, height);
  }
  {
    WaitBlitter();
    /* (A << 4) & 0xF0F0  */
    custom->bltcon0 = (SRCA | DEST) | ANBC | ABC | ASHIFT(shiftamt);
    custom->bltcon1 = BLITREVERSE;
    custom->bltafwm = -1;
    custom->bltalwm = lwm;
    custom->bltamod = (input->width - width) / 2;
    custom->bltdmod = 0;
    custom->bltcdat = 0x0f0f;
    custom->bltadat = 0;

    custom->bltapt = POS4BPP(chunky, input->width, x0+width, y0+width -1) - 2;
    custom->bltdpt = thi + (width*height/2) - 2; //ok
    //BLTSIZE_VAL is in words. 4px per word
    custom->bltsize = BLTSIZE_VAL(width/4, height);
  }
  WaitBlitter();

  Log("CPB done\n");
}



#if 1
#define BLITTERSZ(h, w) ((h << 6) | w)
static void PlanarToSprite(const BitmapT *planar, SpriteT *sprites){
  /* Copy out planar format into sprites
     This function takes care of interlacing SPRxDATA and SPRxDATB registers
     inside a SprDataT structure
  */
  short i = 0;

  for(i = 0; i < 4; i++){
    //Sprite 0, plane 0
    void *sprdat =  sprites[i*2].sprdat->data;
    {
      WaitBlitter();

      custom->bltcon0 = SRCA | DEST | A_TO_D;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 6;
      custom->bltdmod = 2;

      custom->bltapt = planar->planes[0] + 2*i;
      custom->bltdpt = sprdat;
      custom->bltsize = BLTSIZE_VAL(1, HEIGHT);
    }
    //Sprite 0, plane 1
    {
      WaitBlitter();

      custom->bltcon0 = SRCA | DEST | A_TO_D;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 6;
      custom->bltdmod = 2;
      custom->bltadat = 0xFFFF;
      custom->bltapt = planar->planes[1] + 2*i;
      custom->bltdpt = sprdat + 2;
      custom->bltsize = BLTSIZE_VAL(1, HEIGHT);
    }
    WaitBlitter();

    sprdat = sprites[i*2 + 1].sprdat->data;

    //Sprite 1, plane 2
    {
      WaitBlitter();

      custom->bltcon0 = SRCA | DEST | A_TO_D;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 6;
      custom->bltdmod = 2;

      custom->bltapt = planar->planes[2] + 2*i;
      custom->bltdpt = sprdat;
      custom->bltsize = BLTSIZE_VAL(1, HEIGHT);
    }

    //Sprite 1, plane 3
    {
      WaitBlitter();

      custom->bltcon0 = SRCA | DEST | A_TO_D;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 6;
      custom->bltdmod = 2;

      custom->bltapt = planar->planes[3] + 2*i;
      custom->bltdpt = sprdat + 2;
      custom->bltsize = BLTSIZE_VAL(1, HEIGHT);
    }
  }
  WaitBlitter();

}
#endif

PROFILE(UVMapRender);
PROFILE(ChunkyToPlanar);
PROFILE(CropPixmapBlitter);
PROFILE(PlanarToSprite);
static void Render(void) {
  short xo = 0x00;
  short yo = 0x00;
  xo = normfx(SIN(frameCount * 4) * 0x50);
  yo = normfx(COS(frameCount * 7)  * 0x20);
  //yo = normfx(COS(frameCount * 6)  * 0x30);

  //short offset = ((64 - xo) + (64 - yo) * 128) & 16383;
  //u_char *txtHi = textureHi->pixels + offset;
  //u_char *txtLo = textureLo->pixels + offset;

  ProfilerStart(UVMapRender);
  {
    //CropPixmap(&dragon, 0, 112, WIDTH, HEIGHT, segment_p);
    ProfilerStart(CropPixmapBlitter);
    CropPixmapBlitter(dragon_chip, xo+S_WIDTH/2-WIDTH/2 +1, yo+S_HEIGHT/2-HEIGHT/2, WIDTH, HEIGHT, texture_hi, texture_lo);
    //CropPixmapBlitter(dragon_chip, 0, 0, WIDTH, HEIGHT, texture_hi, texture_lo);
    ProfilerStop(CropPixmapBlitter);
    ProfilerStart(UVMapRender);
    UVMapRender(segment_p->pixels, texture_lo, texture_hi);
    ProfilerStop(UVMapRender);

    ProfilerStart(ChunkyToPlanar);
    ChunkyToPlanar(segment_p, segment_bp);
    ProfilerStop(ChunkyToPlanar);

    ProfilerStart(PlanarToSprite);
    PlanarToSprite(segment_bp, sprite);
    ProfilerStop(PlanarToSprite);

    PositionSprite(sprite, xo, yo);
    //PositionSprite(sprite, 0, 0);
    CopListActivate(cp);
  }

  TaskWaitVBlank();
}

EFFECT(Ball, NULL, NULL, Init, Kill, Render, NULL);
