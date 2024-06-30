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

static PixmapT *textureHi, *textureLo;
static PixmapT *segment_p;
static BitmapT *segment_bp;
static BitmapT *dragon_bp;
static BitmapT *screen;
static SprDataT *sprdat;
static SpriteT sprite[8];

#include "data/dragon-bg.c"
#include "data/texture-15.c"
#include "data/ball.c"

static short active = 0;
static CopListT *cp;

#define UVMapRenderSize (WIDTH * HEIGHT / 2 * 10 + 2)
void (*UVMapRender)(u_char *chunky asm("a0"),
                    u_char *textureHi asm("a1"),
                    u_char *textureLo asm("a2"));
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
  CopSetupBitplanes(cp, screen, S_DEPTH); //XXX dragon is now a pixmap
  for (i = 0; i < 8; i++)
    CopInsSetSprite(&sprptr[i], &sprite[i]);
  return CopListFinish(cp);
}

// Attn: in place
void PixmapToBitmap(BitmapT *bm, short width, short height, short depth,
                    void *pixels)
{
  short bytesPerRow = ((short)(width + 15) & ~15) / 8;
  int bplSize = bytesPerRow * height;

  bm->width = width;
  bm->height = height;
  bm->depth = depth;
  bm->bytesPerRow = bytesPerRow;
  bm->bplSize = bplSize;
  //ELF->ST: BM_CPUONLY was BM_DISPLAYABLE
  bm->flags = BM_CPUONLY | BM_STATIC;

  BitmapSetPointers(bm, pixels); // crash

  {
    ////void *planes = MemAlloc(bplSize * 4, MEMF_PUBLIC);
    //c2p_1x1_4( void *chunky, void* bitplanes, short width,
    //           short height, int bplSize)
    ////////c2p_1x1_4(pixels, *(bm->planes), bm->width, bm->height, bplSize);
    c2p_1x1_4(pixels, *(bm->planes), 240     , 228   , bplSize);
    ////memcpy(planes, pixels, bplSize * 4);
    //MemFree(planes); // we wanna keep the bitmap
 
  }
}


static void Init(void) {
  u_short bitplanesz = ((dragon_width + 15) & ~15) / 8 * dragon_height;
  //segment_bp and segment_p are bitmap and pixmap for the magnified segment
  //ELF->ST: BM_CPUONLY was BM_DISPLAYABLE
  segment_bp = NewBitmap(WIDTH, HEIGHT, S_DEPTH, BM_CLEAR);
  segment_p = NewPixmap(WIDTH, HEIGHT, PM_CMAP4, MEMF_CHIP);
  screen = NewBitmap(S_WIDTH, S_HEIGHT, S_DEPTH, BM_CLEAR);

  // c2p requires target bitplanes to be contiguous in memory, therefore we
  // allocate the BitmapT manually
  dragon_bp = (BitmapT *) MemAlloc(sizeof(BitmapT) + 4 * bitplanesz,
				   MEMF_PUBLIC | MEMF_CLEAR);
  
  dragon_bp->width  = dragon_width;
  dragon_bp->height = dragon_height;
  dragon_bp->bytesPerRow = ((dragon_width + 15) & ~15) / 8;
  dragon_bp->bplSize = dragon_bp->bytesPerRow * dragon_bp->height;
  dragon_bp->depth = 4;
  dragon_bp->flags = BM_STATIC;
  dragon_bp->planes[0] = dragon_bp + sizeof(BitmapT);
  dragon_bp->planes[1] = dragon_bp->planes[0] + dragon_bp->bplSize;
  dragon_bp->planes[2] = dragon_bp->planes[1] + dragon_bp->bplSize;
  dragon_bp->planes[3] = dragon_bp->planes[2] + dragon_bp->bplSize;

  Log("sizeof(dragon_pixels) =  $%lx\n", sizeof(dragon_pixels));
  Log("sizeof bitplane =  $%x * depth $%x = total $%lx\n",
      dragon_bp->bplSize, dragon_bp->depth,
      ((long int) dragon_bp->bplSize) * dragon_bp->depth);
  Log("dragon_bp->planes[0] =  $%p\n", dragon_bp->planes[0]);
  Log("dragon_height =  $%x = %d\n", dragon_height, dragon_height);
  Log("dragon_width  =  $%x = %d\n", dragon_width, dragon_width);
  
  c2p_1x1_4(dragon_pixels, dragon_bp->planes[0], dragon_width,
            dragon_height, dragon_bp->bplSize);
  
  //Copy dragon bitmap to background
  memcpy(screen->planes[0], dragon_bp->planes[0],
        S_WIDTH * S_HEIGHT * S_DEPTH / 8);

  ///UVMapRender = MemAlloc(UVMapRenderSize, MEMF_PUBLIC);
  if(0) MakeUVMapRenderCode();

  //textureHi = NewPixmap(texture.width, texture.height * 2,
  //                      PM_CMAP8, MEMF_PUBLIC);
  //textureLo = NewPixmap(texture.width, texture.height * 2,
  //                      PM_CMAP8, MEMF_PUBLIC);

  EnableDMA(DMAF_BLITTER | DMAF_BLITHOG);

  /*sprdat = MemAlloc(SprDataSize(64, 2) * 8 * 2, MEMF_CHIP|MEMF_CLEAR);

  {
    SprDataT *dat = sprdat;
    short j; //i, j;

    //for (i = 0; i < 2; i++)
    for (j = 0; j < 8; j++) {
      MakeSprite(&dat, 64, j & 1, &sprite[j]);
      EndSprite(&dat);
    }
  }
  */
  SetupPlayfield(MODE_LORES, S_DEPTH, X(0), Y(0), S_WIDTH, S_HEIGHT);
  LoadColors(dragon_pal_colors, 0);

  cp = MakeCopperList(0); // PANIC()
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER | DMAF_SPRITE);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_BLITTER | DMAF_SPRITE);

  DeleteCopList(cp);
  DeletePixmap(textureHi);
  DeletePixmap(textureLo);
  MemFree(UVMapRender);
  MemFree(sprdat);
  MemFree(dragon_bp);
  DeletePixmap(segment_p);
  DeleteBitmap(segment_bp);
}

#define BLTSIZE (WIDTH * HEIGHT / 2) //XXX: bad, set to actual pixmap size

#if (BLTSIZE / 4) > 1024
#error "blit size too big!"
#endif
#if 1
#define SPRITEH 64
/* TODO: change output to a pointer to two consecutive sprites in memory */
static void ChunkyToPlanar(PixmapT *input, void *output) {
  void *planes = output;
  void *chunky = input->pixels;

  /* Output format [words]:
     CW1            ; Sprite 0 control words
     CW2            ;
     SPR0DATA       ; Sprite 0 data
     SPR0DATB       ;
     SPR0DATA      
     SPR0DATB
     ...
     EOS            ; End of sprite 0
     CW1            ; Sprite 1 control words
     CW2            ;
     SPR0DATA       ; Sprite 1 data
     SPR0DATB
     ...
     EOS            ; End of sprite 1
  */

  /* Swap 8x4, pass 1. */
  {
    WaitBlitter();

    /* (a & 0xFF00) | ((b >> 8) & ~0xFF00) */
    /* Minterm select: 6 | 7 | 5 | 2: D = AC + AB + B */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC);
    custom->bltcon1 = BSHIFT(8);
    custom->bltafwm = -1;
    custom->bltalwm = -1;
    custom->bltamod = 4;
    custom->bltbmod = 4;
    custom->bltdmod = 4;
    custom->bltcdat = 0xFF00;

    custom->bltapt = chunky;
    custom->bltbpt = chunky + 4;
    custom->bltdpt = planes;
    custom->bltsize = 2 | ((BLTSIZE / 8) << 6);
  }

  /* Swap 8x4, pass 2. */
  {
    WaitBlitter();

    /* ((a << 8) & 0xFF00) | (b & ~0xFF00) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC) | ASHIFT(8);
    custom->bltcon1 = BLITREVERSE;

    custom->bltapt = chunky + BLTSIZE - 6;
    custom->bltbpt = chunky + BLTSIZE - 2;
    custom->bltdpt = planes + BLTSIZE - 2;
    custom->bltsize = 2 | ((BLTSIZE / 8) << 6);
  }

  /* Swap 4x2, pass 1. */
  {
    WaitBlitter();

    /* (a & 0xF0F0) | ((b >> 4) & ~0xF0F0) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC);
    custom->bltcon1 = BSHIFT(4);
    custom->bltamod = 2;
    custom->bltbmod = 2;
    custom->bltdmod = 2;
    custom->bltcdat = 0xF0F0;

    custom->bltapt = planes;
    custom->bltbpt = planes + 2;
    custom->bltdpt = chunky;
    custom->bltsize = 1 | ((BLTSIZE / 4) << 6);
  }

  /* Swap 4x2, pass 2. */
  {
    WaitBlitter();

    /* ((a << 4) & 0xF0F0) | (b & ~0xF0F0) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC) | ASHIFT(4);
    custom->bltcon1 = BLITREVERSE;

    custom->bltapt = planes + BLTSIZE - 4;
    custom->bltbpt = planes + BLTSIZE - 2;
    custom->bltdpt = chunky + BLTSIZE - 2;
    custom->bltsize = 1 | ((BLTSIZE / 4) << 6);
  }
  // start modify for sprite output format

  /* Swap 2x2, pass 1. */
  {
    WaitBlitter();

    /* ((a >> 2) & 0x3333) | (b & ~0x3333) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC);
    custom->bltcon1 = BSHIFT(2);
    custom->bltamod = 4;
    custom->bltbmod = 4;
    custom->bltdmod = 4;
    custom->bltcdat = 0x3333;

    custom->bltapt = chunky;
    custom->bltbpt = chunky + 4;
    custom->bltdpt = planes;
    custom->bltsize = 2 | ((BLTSIZE / 8) << 6);
  }
  /* Swap 2x2, pass 2. */
  {
    WaitBlitter();

    /* ((a << 4) & 0xF0F0) | (b & ~0xF0F0) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC) | ASHIFT(2);
    custom->bltcon1 = BLITREVERSE;

    custom->bltapt = chunky + BLTSIZE - 6;
    custom->bltbpt = chunky + BLTSIZE - 2;
    custom->bltdpt = planes + BLTSIZE - 2;
    custom->bltsize = 2 | ((BLTSIZE / 8) << 6);
	
  }

  /* Swap 1x1, pass 1. */
  {
    WaitBlitter();

    /* ((a >> 1) & 0x5555) | (b & ~0x5555) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC);
    custom->bltcon1 = BSHIFT(1);
    custom->bltamod = 2;
    custom->bltbmod = 2;
    custom->bltdmod = 2;
    custom->bltcdat = 0x5555;

    custom->bltapt = planes;
    custom->bltbpt = planes + 2;
    custom->bltdpt = chunky;
    custom->bltsize = 1 | ((BLTSIZE / 4) << 6);
  }
  /* Swap 1x1, pass 2. */
  
  {
    WaitBlitter();

    /* ((a << 1) & 0x5555) | (b & ~0x5555) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC) | ASHIFT(1);
    custom->bltcon1 = BLITREVERSE;

    custom->bltapt = planes + BLTSIZE - 4;
    custom->bltbpt = planes + BLTSIZE - 2;
    custom->bltdpt = chunky + BLTSIZE - 2;
    custom->bltsize = 1 | ((BLTSIZE / 4) << 6);
  }

  /* Populate SPR0DATA */
  /* Minterms: A */
  {
    WaitBlitter();

    /* ( a ) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | ANBNC);
    custom->bltcon1 = 0;
    custom->bltamod = 3;
    custom->bltbmod = 3;
    custom->bltdmod = 1;
    custom->bltcdat = 0xDEAD;

    custom->bltapt = chunky;
    custom->bltbpt = chunky; //unused
    custom->bltdpt = planes + 2; // Skip 2 control words
    // ta sztósteczka to jest offset wysokości w BLTSIZE,
    // dolne 6 bitow to szerokości blitu w słowach
    custom->bltsize = 1 | (SPRITEH << 6); //XXX
  }
  /* Populate SPR0DATB */
  /* Minterms: A */
  {
    WaitBlitter();

    /* ( a ) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | ANBNC);
    custom->bltcon1 = 0;
    custom->bltamod = 3;
    custom->bltbmod = 3;
    custom->bltdmod = 1;
    custom->bltcdat = 0xDEAD;

    custom->bltapt = chunky + 1;
    custom->bltbpt = chunky; //unused
    custom->bltdpt = planes + 3; // Skip 2 control words and SPR0DATA
    custom->bltsize = 1 | (SPRITEH << 6); //XXX

  }
  /* Populate SPR1DATA */
  /* Minterms: A */
  {
    WaitBlitter();

    /* ( a ) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | ANBNC);
    custom->bltcon1 = 0;
    custom->bltamod = 3;
    custom->bltbmod = 3;
    custom->bltdmod = 1;
    custom->bltcdat = 0xDEAD;

    custom->bltapt = chunky + 2;
    custom->bltbpt = chunky; //unused
    custom->bltdpt = planes + BLTSIZE/2 + 2; // Skip SPR0 and 2 control words
    custom->bltsize = 1 | (SPRITEH << 6); //XXX

  }
  /* Populate SPR1DATB */
  /* Minterms: A */
  {
    WaitBlitter();

    /* ( a ) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | ANBNC);
    custom->bltcon1 = 0;
    custom->bltamod = 3;
    custom->bltbmod = 3;
    custom->bltdmod = 1;
    custom->bltcdat = 0xDEAD;

    custom->bltapt = chunky + 3;
    custom->bltbpt = chunky; //unused
    custom->bltdpt = planes + BLTSIZE/2 + 3; // Skip SPR0, 2 control words and one SPR1DATA
    custom->bltsize = 1 | (SPRITEH << 6); //XXX

  }


  
#if 0  
  /* Swap 2x1, pass 1 & 2. */
  {
    WaitBlitter();

    /* (a & 0xCCCC) | ((b >> 2) & ~0xCCCC) */
    custom->bltamod = 6;
    custom->bltbmod = 6;
    custom->bltdmod = 0;
    custom->bltcdat = 0xCCCC;
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC);
    custom->bltcon1 = BSHIFT(2);

    custom->bltapt = chunky;
    custom->bltbpt = chunky + 4;
    custom->bltdpt = planes + BLTSIZE * 3 / 4;
    custom->bltsize = 1 | ((BLTSIZE / 8) << 6);

    WaitBlitter();
    custom->bltapt = chunky + 2;
    custom->bltbpt = chunky + 6;
    custom->bltdpt = planes + BLTSIZE * 2 / 4;
    custom->bltsize = 1 | ((BLTSIZE / 8) << 6);
  }

  /* Swap 2x1, pass 3 & 4. */
  {
    WaitBlitter();

    /* ((a << 2) & 0xCCCC) | (b & ~0xCCCC) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC) | ASHIFT(2);
    custom->bltcon1 = BLITREVERSE;

    custom->bltapt = chunky + BLTSIZE - 8;
    custom->bltbpt = chunky + BLTSIZE - 4;
    custom->bltdpt = planes + BLTSIZE * 2 / 4 - 2;
    custom->bltsize = 1 | ((BLTSIZE / 8) << 6);

    WaitBlitter();
    custom->bltapt = chunky + BLTSIZE - 6;
    custom->bltbpt = chunky + BLTSIZE - 2;
    custom->bltdpt = planes + BLTSIZE * 1 / 4 - 2;
    custom->bltsize = 1 | ((BLTSIZE / 8) << 6);
  }
#endif
}
#if 0
static void BitmapToSprite(BitmapT *input, SpriteT sprite[8]) {
  void *planes = input->planes[0];
  short bltsize = (input->height << 6) | 1;
  short i = 0;

  WaitBlitter();

  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltcon0 = (SRCA | DEST) | A_TO_D;
  custom->bltcon1 = 0;
  custom->bltamod = 6;
  custom->bltdmod = 2;

  for (i = 0; i < 4; i++) {
    SprDataT *sprdat0 = (sprite++)->sprdat;
    SprDataT *sprdat1 = (sprite++)->sprdat;

    WaitBlitter();
    custom->bltapt = planes + i * 2;
    custom->bltdpt = &sprdat0->data[0][0];
    custom->bltsize = bltsize;

    WaitBlitter();
    custom->bltdpt = &sprdat0->data[0][1];
    custom->bltsize = bltsize;

    WaitBlitter();
    custom->bltdpt = &sprdat1->data[0][0];
    custom->bltsize = bltsize;

    WaitBlitter();
    custom->bltdpt = &sprdat1->data[0][1];
    custom->bltsize = bltsize;
  }
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
#endif
#if 1
static void CropPixmap(PixmapT *input, short x0, short y0, short width, short height, PixmapT* output){
  short j = 0;
  for(j = y0; j < height+y0; j++){
    memcpy(output->pixels + j*output->width + x0,
	   input->pixels + j*input->width,
	   width);
  }
}
#endif

PROFILE(UVMapRender);

static void Render(void) {
  short xo = normfx(SIN(frameCount * 16) * 128);
  short yo = normfx(COS(frameCount * 16) * 100);
  //short offset = ((64 - xo) + (64 - yo) * 128) & 16383;
  //u_char *txtHi = textureHi->pixels + offset;
  //u_char *txtLo = textureLo->pixels + offset;
  
  ProfilerStart(UVMapRender);
  {
    //XXX: dragon.pixels 
    if (0) CropPixmap(dragon.pixels, 16, 16, WIDTH, HEIGHT, segment_p);
    //PixmapToBitmap(segment_bp, WIDTH, HEIGHT, S_DEPTH, dragon.pixels);
    
    
    //(*UVMapRender)(segment_p->pixels, txtHi, txtLo);
    //(*UVMapRender)(dragon_pixels, txtHi, txtLo);
    //ChunkyToPlanar(segment_p, segment_bp);
    if(0) ChunkyToPlanar(segment_p, sprite);

    //BitmapToSprite(segment_bp, sprite);
    if(0) PositionSprite(sprite, xo / 2, yo / 2);
    //CopListActivate(cp);
  }
  ProfilerStop(UVMapRender);

  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Ball, NULL, NULL, Init, Kill, Render, NULL);
