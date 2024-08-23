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

// target screen bitplanes
static BitmapT dragon_bp;
static __data_chip char dragon_bitplanes[S_WIDTH * S_HEIGHT * S_DEPTH / 8];

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
  segment_p = NewPixmap(WIDTH, HEIGHT, PM_CMAP8, MEMF_CHIP);
  screen = NewBitmap(S_WIDTH, S_HEIGHT, S_DEPTH, BM_CLEAR);

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

  //c2p_1x1_4(dragon_pixels, dragon_bp->planes[0], dragon_width,
  //          dragon_height, dragon_bp->bplSize);
  EnableDMA(DMAF_BLITTER | DMAF_BLITHOG);
  Log("&dragon_bp = %p\n", dragon_bp);

  /* *((short*) dragon_bp.planes[0]) = 0x1111;
  *((short*) dragon_bp.planes[0]+1) = 0x2222;
  *((short*) dragon_bp.planes[0]+2) = 0x3333;
  *((short*) dragon_bp.planes[0]+3) = 0x4444;

  *((short*) dragon_bp.planes[1]+128) = 0x5555;
  *((short*) dragon_bp.planes[1]+129) = 0x5555;
  *((short*) dragon_bp.planes[1]+130) = 0x5555;
  *((short*) dragon_bp.planes[1]+131) = 0x5555;
  */

  ChunkyToPlanar(&dragon, &dragon_bp);
  Log("c2p done");

  //Copy dragon bitmap to background
  memcpy(screen->planes[0], dragon_bp.planes[0],
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
#define BLTAREA(w, h, mod) ((2*w + mod) * h)
// Calculate necesary height for the area
#define BLITHEIGHT(w, area, mod) (area / (2*w + mod))

/* C2P pass 1 is AC + BNC, pass 2 is ANC + BC */
#define C2P_LF_PASS1 (NABNC | ANBC | ABNC | ABC)
#define C2P_LF_PASS2 (NABC | ANBNC | ABNC | ABC)

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

  blith = planesz / 4;
  Log("ChunkyToPlanar: bitplane size  = 0x%x\n", planesz );

  Log("ChunkyToPlanar: blit height is = 0x%x\n", blith );


  // This is implemented as in prototypes/c2p/c2p_1x1_4bp_blitter_backforth.py
  // rem: modulos and pointers are in bytes

  //TODO: Specialcase last non-1024h blit
  //TODO: Parametrize the blit sizes for screen size other than full screen

  while(blitarea < planarsz){
    // Swap 8x4, pass 1
    {
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
      custom->bltsize = BLTSIZE_VAL(2, 0);
    }

    // Swap 8x4, pass 2
    {
      WaitBlitter();

      /* ((a << 8) & ~0x00FF) | (b & 0x00FF) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS2 | ASHIFT(8);
      custom->bltcon1 = BLITREVERSE;

      custom->bltapt = chunky - 6 + BLTAREA(2, 1024, 4) + blitarea;
      custom->bltbpt = chunky - 2 + BLTAREA(2, 1024, 4) + blitarea;
      custom->bltdpt = planes - 2 + BLTAREA(2, 1024, 4) + blitarea;
      custom->bltsize = BLTSIZE_VAL(2, 0);
    }
    blitarea += BLTAREA(2, 1024, 4);
    Log("8x4: Blitarea = %x\n", blitarea);
  }

  blitarea = 0;
  while(blitarea < planarsz){
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
      custom->bltsize = BLTSIZE_VAL(1, 0);
    }

    // Swap 4x2, pass 2
    {
      WaitBlitter();

      /* ((a << 4) & ~0x0F0F) | (b & 0x0F0F) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS2 | ASHIFT(4);
      custom->bltcon1 = BLITREVERSE;

      custom->bltapt = planes + BLTAREA(1, 1024, 2) - 4 + blitarea;
      custom->bltbpt = planes + BLTAREA(1, 1024, 2) - 2 + blitarea;
      custom->bltdpt = chunky + BLTAREA(1, 1024, 2) - 2 + blitarea;
      custom->bltsize = BLTSIZE_VAL(1, 0);
    }

    blitarea += BLTAREA(1, 1024, 2);
    Log("4x2: Blitarea = %x\n", blitarea);
  }


  blitarea = 0;
  while(blitarea < planarsz){
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
      custom->bltsize = BLTSIZE_VAL(2, 0);
    }

    // Swap 2x2, pass 2
    {
      WaitBlitter();

      /* ((a << 2) & ~0x3333) | (b & 0x3333) */
      custom->bltcon0 = (SRCA | SRCB | DEST) | C2P_LF_PASS2 | ASHIFT(2);
      custom->bltcon1 = BLITREVERSE;
      //custom->bltadat = 0xFFFF;
      //custom->bltbdat = 0x0000;

      custom->bltapt = chunky - 6 + BLTAREA(2, 1024, 4) + blitarea;
      custom->bltbpt = chunky - 2 + BLTAREA(2, 1024, 4) + blitarea;
      custom->bltdpt = planes - 2 + BLTAREA(2, 1024, 4) + blitarea;
      custom->bltsize = BLTSIZE_VAL(2, 0);
    }

    blitarea += BLTAREA(2, 1024, 4);
    Log("2x2: Blitarea = %x\n", blitarea);
  }

  // Copy to bitplanes - last swap
  // Numbers in quotes refer to bitplane numbers on test card.

  blitarea = 0;
  while(blitarea < planarsz){
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
      custom->bltdpt = chunky + 3*planesz + i*BLTAREA(1, 512, 0);
      custom->bltsize = BLTSIZE_VAL(1, 512);
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

      custom->bltapt = planes + BLTAREA(1, 512, 6) - 8 + blitarea;
      custom->bltbpt = planes + BLTAREA(1, 512, 6) - 6 + blitarea;
      custom->bltdpt = chunky + 2*planesz - 2 + (i+1)*BLTAREA(1, 512, 0);

      custom->bltsize = BLTSIZE_VAL(1, 512);
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
      custom->bltdpt = chunky + planesz + i*BLTAREA(1, 512, 0);
      custom->bltsize = BLTSIZE_VAL(1, 512);
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

      custom->bltapt = planes + BLTAREA(1, 512, 6) - 4 + blitarea;
      custom->bltbpt = planes + BLTAREA(1, 512, 6) - 2 + blitarea;
      custom->bltdpt = chunky - 2 + (i+1)*BLTAREA(1, 512, 0);
      custom->bltsize = BLTSIZE_VAL(1, 512);
    }
    WaitBlitter();

#endif
    blitarea += BLTAREA(1, 512, 6);
    Log("bplcpy: Blitarea AB = %x, blitarea D = %x\n",
	blitarea, i * BLTAREA(1, 512, 0));
    i++;

  }

  //memset(planes, 0, 0xa000);
  //Because there is an even number of steps here, c2p finishes with planar data
  //in chunky. These last 4 blits move it to planar, where it can be copied to
  //screen.

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
    custom->bltsize = BLTSIZE_VAL(10, 512);
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
    custom->bltsize = BLTSIZE_VAL(10, 512);
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
    custom->bltsize = BLTSIZE_VAL(10, 512);
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
    custom->bltsize = BLTSIZE_VAL(10, 512);
  }
#endif
  return;

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
  //XXX: make this with blitter, make this 4bpp
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
