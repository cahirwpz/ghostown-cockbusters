#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <sprite.h>
#include <system/memory.h>
#include <system/interrupt.h>
#include <common.h>

#define WIDTH  320
#define HEIGHT 256
#define DEPTH  3

#define N 1
#define VERTICAL    0
#define HORIZONTAL  1
#define NO_OF_LINES 3


static CopListT *cp;
static CopInsPairT *bplptr;
static BitmapT *screen[2];
static short active = 0;

static short horizont[256] = {0};
static short h_lines[NO_OF_LINES] = {16, 128, 196};

static short __data_chip vertical[3][20];
static short v_lines[NO_OF_LINES] = {60, 160, 260};

static short __data_chip carry[2][(WIDTH/16)*HEIGHT] = {{0}};
static short c = 0;

static u_short LINE[3] = {
  85 << 9,  // bin: 101010101
  54 << 9,  // bin: 011000110
   8 << 9,  // bin: 000111000
};

// static u_short LINE[3] = {
//   341 << 7,  // bin: 101010101
//   198 << 7,  // bin: 011000110
//    56 << 7,  // bin: 000111000
// };


static void DoMagic(void **planes) {
  (void)DoMagic;
  (void)planes;
  (void)carry;
  (void)c;

  custom->bltamod = -40;
  custom->bltbmod = 0;
  custom->bltcmod = 0;
  custom->bltdmod = 0;

  custom->bltadat = -1;
  custom->bltbdat = -1;
  custom->bltcdat = -1;
  custom->bltddat = -1;

  custom->bltafwm = -1;
  custom->bltalwm = -1;

  /* BITPLANE 0 */
  /* CARRY */
  custom->bltapt = vertical[0];
  custom->bltbpt = planes[0];
  custom->bltdpt = carry[0];

  custom->bltcon0 = (SRCA | SRCB | DEST ) | A_AND_B;
  custom->bltcon1 = 0;

  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();
  /* SUM */
  custom->bltapt = vertical[0];
  custom->bltbpt = planes[0];
  custom->bltdpt = planes[0];

  custom->bltcon0 = (SRCA | SRCB | DEST) | A_OR_B;
  custom->bltcon1 = 0;

  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();

  /* BITPLANE 1 */
  /* CARRY */
  custom->bltapt = vertical[1];
  custom->bltbpt = planes[1];
  custom->bltcpt = carry[0];
  custom->bltdpt = carry[1];

  custom->bltcon0 = (SRCA | SRCB | SRCC | DEST ) | FULL_ADDER_CARRY;
  custom->bltcon1 = 0;

  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();
  /*SUM */
  custom->bltapt = vertical[1];
  custom->bltbpt = planes[1];
  custom->bltcpt = carry[0];
  custom->bltdpt = planes[1];

  custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | FULL_ADDER;
  custom->bltcon1 = 0;

  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();

  /* BITPLANE 2 */
  /* CARRY */
  custom->bltapt = vertical[2];
  custom->bltbpt = planes[2];
  custom->bltcpt = carry[1];
  custom->bltdpt = carry[0];

  custom->bltcon0 = (SRCA | SRCB | SRCC | DEST ) | FULL_ADDER_CARRY;
  custom->bltcon1 = 0;

  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();
  /* SUM */
  custom->bltapt = vertical[2];
  custom->bltbpt = planes[2];
  custom->bltcpt = carry[1];
  custom->bltdpt = planes[2];

  custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | FULL_ADDER;
  custom->bltcon1 = 0;

  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();

  /* PROPAGATE CARRY */
  custom->bltamod = 0;

  custom->bltapt = carry[0];
  custom->bltbpt = planes[0];
  custom->bltdpt = planes[0];

  custom->bltcon0 = (SRCA | SRCB | DEST) | A_OR_B;
  custom->bltcon1 = 0;

  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();

  custom->bltapt = carry[0];
  custom->bltbpt = planes[1];
  custom->bltdpt = planes[1];

  custom->bltcon0 = (SRCA | SRCB | DEST) | A_OR_B;
  custom->bltcon1 = 0;

  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();

  custom->bltapt = carry[0];
  custom->bltbpt = planes[2];
  custom->bltdpt = planes[2];

  custom->bltcon0 = (SRCA | SRCB | DEST) | A_OR_B;
  custom->bltcon1 = 0;

  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();
}


/* HORIZONTAL */
static void __ClearLine(void **planes, short pos, short thickness) {
  if (pos > 256) {
    return;
  }
  if (pos + thickness >= 256) {
    thickness = 256 - pos;
  }
  if (pos < 0) {
    thickness += pos;
    pos = 0;
  }
  if (thickness < 1) {
    return;
  }

  custom->bltamod = 0;
  custom->bltbmod = 0;
  custom->bltcmod = 0;
  custom->bltdmod = 0;

  custom->bltafwm = -1;
  custom->bltalwm = -1;

  custom->bltdpt = *planes + pos*40;

  custom->bltcon0 = DEST | 0x00;
  custom->bltcon1 = 0;
  custom->bltsize = (thickness << 6) | 20;

  WaitBlitter();
}

static void ClearLines(void **planes) {
  short i = 0;
  short pos = 0;

  for (i = 0; i < NO_OF_LINES; ++i) {
    pos = h_lines[i];
    __ClearLine(&planes[0], pos-N, N);
    __ClearLine(&planes[1], pos-N, N);
    __ClearLine(&planes[2], pos-N, N);

    __ClearLine(&planes[0], pos, N);
    __ClearLine(&planes[1], pos, N);
    __ClearLine(&planes[2], pos, N);

    __ClearLine(&planes[0], pos+N, N);
    __ClearLine(&planes[1], pos+N, N);
    __ClearLine(&planes[2], pos+N, N);
    
    __ClearLine(&planes[0], pos+N*2, N);
    __ClearLine(&planes[1], pos+N*2, N);
    __ClearLine(&planes[2], pos+N*2, N);
    
    __ClearLine(&planes[0], pos+N*3, N);
    __ClearLine(&planes[1], pos+N*3, N);
    __ClearLine(&planes[2], pos+N*3, N);
    
    __ClearLine(&planes[0], pos+N*4, N);
    __ClearLine(&planes[1], pos+N*4, N);
    __ClearLine(&planes[2], pos+N*4, N);
    
    __ClearLine(&planes[0], pos+N*5, N);
    __ClearLine(&planes[1], pos+N*5, N);
    __ClearLine(&planes[2], pos+N*5, N);
    
    __ClearLine(&planes[0], pos+N*6, N);
    __ClearLine(&planes[1], pos+N*6, N);
    __ClearLine(&planes[2], pos+N*6, N);
    
    __ClearLine(&planes[0], pos+N*7, N);
    __ClearLine(&planes[1], pos+N*7, N);
    __ClearLine(&planes[2], pos+N*7, N);
    
    __ClearLine(&planes[0], pos+N*8, N);
    __ClearLine(&planes[1], pos+N*8, N);
    __ClearLine(&planes[2], pos+N*8, N);
    
    __ClearLine(&planes[0], pos+N*9, N);
    __ClearLine(&planes[1], pos+N*9, N);
    __ClearLine(&planes[2], pos+N*9, N);
    
  }
}

static void __BlitHorizontal(void **planes, short pos, short thickness) {
  if (pos > 256) {
    return;
  }
  if (pos + thickness >= 256) {
    thickness = 256 - pos;
  }
  if (pos < 0) {
    thickness += pos;
    pos = 0;
  }
  if (thickness < 1) {
    return;
  }

  custom->bltamod = 0;
  custom->bltbmod = 0;
  custom->bltcmod = 0;
  custom->bltdmod = 0;

  custom->bltafwm = -1;
  custom->bltalwm = -1;

  custom->bltdpt = *planes + pos*40;

  custom->bltcon0 = DEST | 0xFF;
  custom->bltcon1 = 0;
  custom->bltsize = (thickness << 6) | 20;

  WaitBlitter();
}

static void BlitLine(void **planes, short pos, short brightness) {
  switch (brightness) {
    case 0:
      break;

    case 1:
      __BlitHorizontal(&planes[0], pos, N);
      break;

    case 2:
      __BlitHorizontal(&planes[1], pos, N);
      break;

    case 3:
      __BlitHorizontal(&planes[0], pos, N);
      __BlitHorizontal(&planes[1], pos, N);
      break;

    case 4:
      __BlitHorizontal(&planes[2], pos, N);
      break;
    
    case 5:
      __BlitHorizontal(&planes[0], pos, N);
      __BlitHorizontal(&planes[2], pos, N);
      break;

    case 6:
      __BlitHorizontal(&planes[1], pos, N);
      __BlitHorizontal(&planes[2], pos, N);
      break;

    case 7:
    default:
      __BlitHorizontal(&planes[0], pos, N);
      __BlitHorizontal(&planes[1], pos, N);
      __BlitHorizontal(&planes[2], pos, N);
  }
}

static void CalculateAndDrawHorizontalLines(void** planes) {
  short i = 0;
  short pos = 0;

  for (i = 0; i < 256; ++i) {
    horizont[i] = 0;
  }

  pos =h_lines[0];

  horizont[pos+0] = 1;
  horizont[pos+1] = 2;
  horizont[pos+2] = 3;
  horizont[pos+3] = 4;
  horizont[pos+4] = 3;
  horizont[pos+5] = 2;
  horizont[pos+6] = 1;

  for (i = 1; i < NO_OF_LINES; ++i) {
    pos = h_lines[i];

    horizont[pos+0] += 1;
    horizont[pos+1] += 2;
    horizont[pos+2] += 3;
    horizont[pos+3] += 4;
    horizont[pos+4] += 3;
    horizont[pos+5] += 2;
    horizont[pos+6] += 1;
  }

  for (i = 0; i < 256; ++i) {
    BlitLine(planes, i, horizont[i]);
  }
}

static void HorizontalTest(void) {
  (void)ClearLines;
  // ClearLines(screen[active]->planes);
  // h_lines[0]++;
  // if (h_lines[0] > 200) {
  //   h_lines[0] = 16;
  // }
  // h_lines[2]--;
  // if (h_lines[2] < 16) {
  //   h_lines[2] = 196;
  // }
  CalculateAndDrawHorizontalLines(screen[active]->planes);
}


/* VERTICAL */
static void CalculateFirstLine(void) {
  short word, offset, aux;
  short *ptr;
  short i, j;

  /* Set first line to 0 */
  for (i = 0; i < 3; ++i) {
    ptr = vertical[i];
    for (j = 0; j < 20; ++j) {
      ptr[j] = 0;
    }
  }

  /* Calculate coordinates */
  word = v_lines[0]/16;
  offset = v_lines[0] - (word * 16);

  /* Draw first beam */
  for (i = 0; i < 3; ++i) {
    ptr = vertical[i];
    ptr[word] = LINE[i] >> offset;
    if (offset > 7) {
      ptr[word+1] = LINE[i] << (16 - offset);
    }
  }

  /* Add rest of the beams */
  for (i = 1; i < NO_OF_LINES; ++i) {
    u_short w1, w2, c1, c2 = 0;

    word = v_lines[i]/16;
    offset = v_lines[i] - (word * 16);

    ptr = vertical[0];
    w1 = LINE[0] >> offset;
    c1 = ptr[word] & w1;
    ptr[word] = ptr[word] ^ w1;
    if (offset > 7) {
      w2 = LINE[0] << (16 - offset);
      c2 = ptr[word+1] & w2;
      ptr[word+1] = ptr[word+1] ^ w2;
    }

    ptr = vertical[1];
    w1 = LINE[1] >> offset;
    /* circural dependency */
    aux = ptr[word];
    ptr[word] = (ptr[word] ^ w1) ^ c1;
    c1 = ((aux ^ w1) & c1) ^ (aux & w1);
    if (offset > 7) {
      w2 = LINE[1] << (16 - offset);
      aux = ptr[word+1];
      ptr[word+1] = ptr[word+1] ^ w2 ^ c2;
      c2 = ((aux ^ w2) & c2) ^ (aux & w2);
    }

    ptr = vertical[2];
    w1 = LINE[2] >> offset;
    aux = ptr[word];
    ptr[word] = ptr[word] ^ w1 ^ c1;
    c1 = ((aux ^ w1) & c1) ^ (aux & w1);
    if (offset > 7) {
      w2 = LINE[2] << (16 - offset);
      aux = ptr[word+1];
      ptr[word+1] = ptr[word+1] ^ w2 ^ c2;
      c2 = ((aux ^ w2) & c2) ^ (aux & w2);
    }
    
    ptr[word] |= c1;
    ptr[word+1] |= c2;
    ptr = vertical[0];
    ptr[word] |= c1;
    ptr[word+1] |= c2;
    ptr = vertical[1];
    ptr[word] |= c1;
    ptr[word+1] |= c2;
  }
}

static void CopyFirstLine(void** planes) {
  short i, j;
  short *ptr;

  for (i = 0; i < 3; ++i) {
    ptr = planes[i];
    for (j = 0; j < 20; ++j) {
      ptr[j] = vertical[i][j];
    }
  }
}

static void VerticalFill(void** planes) {
  custom->bltamod = 0;
  custom->bltbmod = 0;
  custom->bltdmod = 0;
  custom->bltcdat = -1;
  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltcon1 = 0;

  /* BITPLANE 0 */
  custom->bltapt = planes[0];
  custom->bltdpt = planes[0] + WIDTH/8;

  custom->bltcon0 = (SRCA | DEST) | A_TO_D;
  custom->bltsize = ((HEIGHT - 1) << 6) | 20;
  WaitBlitter();

  /* BITPLANE 1 */
  custom->bltapt = planes[1];
  custom->bltdpt = planes[1] + WIDTH/8;

  custom->bltcon0 = (SRCA | DEST) | A_TO_D;
  custom->bltsize = ((HEIGHT - 1) << 6) | 20;
  WaitBlitter();

  /* BITPLANE 2 */
  custom->bltapt = planes[2];
  custom->bltdpt = planes[2] + WIDTH/8;

  custom->bltcon0 = (SRCA | SRCB | DEST) | A_TO_D;
  custom->bltsize = ((HEIGHT - 1) << 6) | 20;
  WaitBlitter();
}


static void ClearBitplanes(void **planes) {
  custom->bltdmod = 0;
  custom->bltcon0 = (DEST) | 0x0;
  custom->bltcon1 = 0;

  custom->bltdpt = planes[0];
  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();

  custom->bltdmod = 0;
  custom->bltcon0 = (DEST) | 0x0;
  custom->bltcon1 = 0;

  custom->bltdpt = planes[1];
  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();

  custom->bltdmod = 0;
  custom->bltcon0 = (DEST) | 0x0;
  custom->bltcon1 = 0;

  custom->bltdpt = planes[2];
  custom->bltsize = (HEIGHT << 6) | 20;
  WaitBlitter();
}


static CopListT *MakeCopperList(void) {
  cp = NewCopList(128);
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);

  CopListFinish(cp);

  return cp;
}

static void Init(void) {
  EnableDMA(DMAF_BLITTER | DMAF_BLITHOG | DMAF_RASTER);  // DMAF_BLITHOG

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, 256);

  SetColor(0, 0x000);
  SetColor(1, 0x111);
  SetColor(2, 0x333);
  SetColor(3, 0x555);
  SetColor(4, 0x777);
  SetColor(5, 0x999);
  SetColor(6, 0xBBB);
  SetColor(7, 0xDDD);

  cp = MakeCopperList();
  CopListActivate(cp);
}

static void Kill(void) {
  DisableDMA(DMAF_BLITTER | DMAF_RASTER);

  DeleteCopList(cp);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

PROFILE(Horiz);
PROFILE(Verti);
PROFILE(Magic);
PROFILE(Total);

static void Render(void) {
  (void)ClearBitplanes;
  (void)HorizontalTest;
  (void)VerticalFill;
  (void)CopyFirstLine;
  (void)carry;

  ProfilerStart(Total);
  ClearBitplanes(screen[active]->planes);

  ProfilerStart(Verti);
  {
    CalculateFirstLine();
    // CopyFirstLine(screen[active]->planes);
    // VerticalFill(screen[active]->planes);
    v_lines[0]++;
    if (v_lines[0] > WIDTH - 10) {
      v_lines[0] = 0;
    }
  }
  ProfilerStop(Verti);

  ProfilerStart(Horiz);
  {
    HorizontalTest();
    h_lines[0]++;
    if (h_lines[0] > HEIGHT - 10) {
      h_lines[0] = 0;
    }
  }
  ProfilerStop(Horiz);

  ProfilerStart(Magic);
  {
    DoMagic(screen[active]->planes);
  }
  ProfilerStop(Magic);

  ProfilerStop(Total);  

  ITER(i, 0, DEPTH - 1, CopInsSet32(&bplptr[i], screen[active]->planes[i]));

  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Darkroom, NULL, NULL, Init, Kill, Render, NULL);