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
