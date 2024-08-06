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

//static PixmapT *textureHi, *textureLo;
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
  CopSetupBitplanes(cp, screen, S_DEPTH);
  for (i = 0; i < 8; i++)
    CopInsSetSprite(&sprptr[i], &sprite[i]);
  return CopListFinish(cp);
}


//XXX This function is currently NOT used
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
    
    c2p_1x1_4(pixels, *(bm->planes), 240     , 228   , bplSize);
    
 
  }
}


static void Init(void) {
  u_short bitplanesz = ((dragon_width + 15) & ~15) / 8 * dragon_height;
  //segment_bp and segment_p are bitmap and pixmap for the magnified segment
  //ELF->ST: BM_CPUONLY was BM_DISPLAYABLE
  segment_bp = NewBitmap(WIDTH, HEIGHT, S_DEPTH, BM_CLEAR);
  segment_p = NewPixmap(WIDTH, HEIGHT, PM_CMAP8, MEMF_CHIP);
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
  
  //c2p_1x1_4(dragon_pixels, dragon_bp->planes[0], dragon_width,
  //          dragon_height, dragon_bp->bplSize);
  EnableDMA(DMAF_BLITTER | DMAF_BLITHOG);

  ChunkyToPlanar(dragon, dragon_bp);
  
  //Copy dragon bitmap to background
  
  memcpy(screen->planes[0], dragon_bp->planes[0],
        S_WIDTH * S_HEIGHT * S_DEPTH / 8);
  

  
  ///UVMapRender = MemAlloc(UVMapRenderSize, MEMF_PUBLIC);
  if(0) MakeUVMapRenderCode();

  //textureHi = NewPixmap(texture.width, texture.height * 2,
  //                      PM_CMAP8, MEMF_PUBLIC);
  //textureLo = NewPixmap(texture.width, texture.height * 2,
  //                      PM_CMAP8, MEMF_PUBLIC);


  sprdat = MemAlloc(SprDataSize(64, 2) * 8 * 2, MEMF_CHIP|MEMF_CLEAR);

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
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER | DMAF_SPRITE);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_BLITTER | DMAF_SPRITE);

  DeleteCopList(cp);
  //DeletePixmap(textureHi);
  //DeletePixmap(textureLo);
  MemFree(UVMapRender);
  MemFree(sprdat);
  MemFree(dragon_bp);
  DeletePixmap(segment_p);
  DeleteBitmap(segment_bp);
}


// Blitter ops as in prototypes/c2p/c2p_1x1_4bpl_sprites.py
#if 0

#define P2S_M0 0x00FF
#define P2S_M1 0x0F0F
#define P2S_M2 0x3333
#define P2S_M3 0x5555
#define BLTSIZE (WIDTH * HEIGHT / 2) //XXX: bad, set to actual pixmap size
static void PixmapToSprites(PixmapT *input, BitmapT *output) {
  void *planes = output->planes[0];
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
     ...            ; Sprites 2-7 in the same format.
  */
  //
  //Blit(lambda a, b: ((a >> 8) & m0) | (b & ~m0),
  //       N // 4, 2, Channel(A, 2, 2), Channel(A, 0, 2), Channel(B, 0, 2))
  //       BLTSIZE
  // Channel(data, start, mod)  ==  bltXpt = source + start; bltXmod = mod
  
  // Pass 1 - Swap 8x4, blit 1
  {
    //WaitBlitter();
    // a >> 8 & M0 | b & ~M0 == a >> 8 & M0 | ~b & M0
    // Minterm select: AC | NBC: 7 - 5 - - - 1 -
    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) |
      (ABC | ANBC | NANBC) |
      ASHIFT(8);
    custom->bltcon1 = 0;
    // custom->bltcon1 = BLITREVERSE; zeby shift w lewo
    custom->bltafwm = -1;
    custom->bltalwm = -1;
    custom->bltamod = 2;
    custom->bltbmod = 2;
    custom->bltdmod = 2;
    custom->bltcdat = P2S_M0;

    custom->bltapt = chunky + 2;
    custom->bltbpt = chunky;
    custom->bltdpt = planes;
    custom->bltsize = 2 | ((BLTSIZE / 4) << 6);
    
  }
  // Pass 1 - Swap 8x4, blit 2
  {
    WaitBlitter();
    // a << 8 & ~M0 | b & M0 == ~a << 8 & M0 | b & M0
    // Minterm select: NAC | BC: 7 - - - 3 - 1
    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) |
      (ABC | NABC | NANBC) |
      ASHIFT(8);
    custom->bltcon1 = BLITREVERSE;
    custom->bltafwm = -1;
    custom->bltalwm = -1;
    custom->bltamod = 2;
    custom->bltbmod = 2;
    custom->bltdmod = 2;
    custom->bltcdat = P2S_M0;

    custom->bltapt = chunky;
    custom->bltbpt = chunky + 2;
    custom->bltdpt = planes + 2;
    custom->bltsize = 2 | ((BLTSIZE / 4) << 6);
  }
  WaitBlitter();
}
#endif
//#if (BLTSIZE / 4) > 1024
//#error "blit size too big!"
//#endif
#define SPRITEH 64
#if 1
static void ChunkyToPlanar(PixmapT *input, BitmapT *output) {
  void *planes = output->planes[0];
  void *chunky = input->pixels;
  u_short BLTSIZE = input->width * input->height / 2;

  //przepisać tutaj c2p_1x1_4bpl_sprites.py


  
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

    custom->bltapt = chunky + BLTSIZE - 2;
    custom->bltbpt = chunky + BLTSIZE - 6;
    custom->bltdpt = planes + BLTSIZE - 6;
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

    custom->bltapt = planes + BLTSIZE - 2;
    custom->bltbpt = planes + BLTSIZE - 4;
    custom->bltdpt = chunky + BLTSIZE - 4;
    custom->bltsize = 1 | ((BLTSIZE / 4) << 6);
  }

  /* Swap 2x2, pass 1. */
  {
    WaitBlitter();

    /* ((a >> 2) & ~0xCCCC) | (b & 0xCCCC) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC) | ASHIFT(2);
    custom->bltamod = 4;
    custom->bltbmod = 4;
    custom->bltdmod = 4;
    custom->bltcdat = 0xCCCC;

    custom->bltapt = chunky + 4;
    custom->bltbpt = chunky;
    custom->bltdpt = planes;
    custom->bltsize = 2 | ((BLTSIZE / 8) << 6);
  }

  
  /* Swap 2x2, pass 2. */
  {
    WaitBlitter();

    /* ((a << 2) & 0xCCCC) | (b & ~0xCCCC) */
    custom->bltcon0 = (SRCA | SRCB | DEST) | (ABC | ABNC | ANBC | NABNC) | ASHIFT(2);
    custom->bltcon1 = BLITREVERSE;

    custom->bltapt = chunky + BLTSIZE - 2;
    custom->bltbpt = chunky + BLTSIZE - 6;
    custom->bltdpt = planes + BLTSIZE - 6;
    custom->bltsize = 2 | ((BLTSIZE / 8) << 6);

  }
  WaitBlitter();
}
#endif
#if 0
static void BitmapToSprite(BitmapT *input, SpriteT sprite[8]) {
  //void *planes = input->planes[0];
  short bltsize = (input->height << 6) | 1;
  short i = 0;

  WaitBlitter();

  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltcon0 = (SRCA | DEST) | A_TO_D;
  custom->bltcon1 = 0;
  custom->bltamod = 1;
  custom->bltdmod = 3;

  for (i = 0; i < 4; i++) {
    SprDataT *sprdat0 = (sprite++)->sprdat;
    SprDataT *sprdat1 = (sprite++)->sprdat;

    WaitBlitter();
    custom->bltapt = input->planes[0];
    custom->bltdpt = &sprdat0->data[0][0];
    custom->bltsize = bltsize;
    
    WaitBlitter();
    custom->bltapt = input->planes[1];
    custom->bltdpt = &sprdat0->data[0][1];
    custom->bltsize = bltsize;
    
    WaitBlitter();
    custom->bltapt = input->planes[2];
    custom->bltdpt = &sprdat1->data[0][0];
    custom->bltsize = bltsize;

    WaitBlitter();
    custom->bltapt = input->planes[3];

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

static void CropPixmap(const PixmapT *input, u_short x0, u_short y0,
		       short width, short height, PixmapT* output){
  u_short j = 0;
  u_short k = 0;
  for(j = y0; j < height+y0; j++){
    memcpy(output->pixels + k*output->width,
	   input->pixels + j*input->width + x0,
	   width);
    k++;
  }
}
#if 0
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
      custom->bltsize = BLITTERSZ(HEIGHT, 1);
    }

    //Sprite 0, plane 1
    {
      WaitBlitter();
    
      custom->bltcon0 = SRCA | DEST | A_OR_B;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 6;
      custom->bltdmod = 2;

      custom->bltapt = planar->planes[1] + 2*i;
      custom->bltdpt = sprdat + 2;
      custom->bltsize = BLITTERSZ(HEIGHT, 1);
    }
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
      custom->bltsize = BLITTERSZ(HEIGHT, 1);
    }
    
    //Sprite 1, plane 3
    {
      WaitBlitter();
    
      custom->bltcon0 = SRCA | DEST | A_OR_B;
      custom->bltafwm = -1;
      custom->bltalwm = -1;
      custom->bltamod = 6;
      custom->bltdmod = 2;

      custom->bltapt = planar->planes[3] + 2*i;
      custom->bltdpt = sprdat + 2;
      custom->bltsize = BLITTERSZ(HEIGHT, 1);
    }
  }
  WaitBlitter();
}
#endif 

//PROFILE(UVMapRender);
//PROFILE(PlanarToSprite);
static void Render(void) {
  //short xo = 0x40; 
  //short yo = 0x40;
  //short xo = normfx(SIN(frameCount * 8) * 128);
  //xshort yo = normfx(COS(frameCount * 7)  * 100);
  //short offset = ((64 - xo) + (64 - yo) * 128) & 16383;
  //u_char *txtHi = textureHi->pixels + offset;
  //u_char *txtLo = textureLo->pixels + offset;
  
  //ProfilerStart(UVMapRender);
  {
    //CropPixmap(&dragon, 0, 0, WIDTH, HEIGHT, segment_p);
    
    //ChunkyToPlanar(segment_p, segment_bp);
    
    //PositionSprite(sprite, xo / 2, yo / 2);
    //PositionSprite(sprite, 20, 20);
    //CopListActivate(cp);
  }
  //ProfilerStop(UVMapRender);

  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Ball, NULL, NULL, Init, Kill, Render, NULL);
