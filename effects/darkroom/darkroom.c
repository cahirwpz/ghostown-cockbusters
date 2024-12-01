#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <sprite.h>
#include <system/memory.h>
#include <system/interrupt.h>
#include <common.h>

#define WIDTH  320
#define HEIGHT 256 // 128
#define DEPTH  3

#define V_LINE_WIDTH  12
#define H_LINE_WIDTH  7
#define NO_OF_LINES 6


static CopListT *cp;
static CopInsPairT *bplptr;
static BitmapT *screen[2];
static short active = 0;


static short v_lines[NO_OF_LINES] = {0, 70, 140, 166, 236, 306};
static short h_lines[NO_OF_LINES] = {0, 60, 120, 129, 189, 249};

static short __data_chip carry[2][H_LINE_WIDTH * (WIDTH/16)] = {{0}};

static u_short V_LINE[3] = {
  13107 << 2,  // bin: 11001100110011
   3900 << 2,  // bin: 00111100111100
    192 << 2,  // bin: 00000011000000
};

// static u_short V_LINE[3] = {
//   85 << 9,  // bin: 1010101
//   54 << 9,  // bin: 0110110
//    8 << 9,  // bin: 0001000
// };

// static u_short V_LINE[3] = {
//   341 << 7,  // bin: 101010101
//   198 << 7,  // bin: 011000110
//    56 << 7,  // bin: 000111000
// };

static short __data_chip H_LINE[3][7][20] = {
  {
    {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},
    {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},
    {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},
    {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,},
  },
  {
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},
    {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,},
    {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},
    {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,},
    {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},
  },
  {
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},
    {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,},
  },
};


static void DoMagic(void **planes) {
  short i = 0;

  custom->bltamod = 0;
  custom->bltbmod = 0;
  custom->bltcmod = 0;
  custom->bltdmod = 0;

  custom->bltadat = -1;
  custom->bltbdat = -1;
  custom->bltcdat = -1;
  custom->bltddat = -1;

  custom->bltafwm = -1;
  custom->bltalwm = -1;

  custom->bltcon1 = 0;

  for (i = 0; i < NO_OF_LINES; ++i) {
    short pos = h_lines[i] * 40;
    /* BITPLANE 0 */
    /* CARRY */
    custom->bltapt = H_LINE[0];
    custom->bltbpt = planes[0] + pos;
    custom->bltdpt = carry[0];

    custom->bltcon0 = HALF_ADDER_CARRY;

    custom->bltsize = (H_LINE_WIDTH << 6) | 20;
    WaitBlitter();
    /* SUM */
    custom->bltapt = H_LINE[0];
    custom->bltbpt = planes[0] + pos;
    custom->bltdpt = planes[0] + pos;

    custom->bltcon0 = HALF_ADDER;

    custom->bltsize = (H_LINE_WIDTH << 6) | 20;
    WaitBlitter();

    /* BITPLANE 1 */
    /* CARRY */
    custom->bltapt = H_LINE[1];
    custom->bltbpt = planes[1] + pos;
    custom->bltcpt = carry[0];
    custom->bltdpt = carry[1];

    custom->bltcon0 = FULL_ADDER_CARRY;

    custom->bltsize = (H_LINE_WIDTH << 6) | 20;
    WaitBlitter();
    /* SUM */
    custom->bltapt = H_LINE[1];
    custom->bltbpt = planes[1] + pos;
    custom->bltcpt = carry[0];
    custom->bltdpt = planes[1] + pos;

    custom->bltcon0 = FULL_ADDER;

    custom->bltsize = (H_LINE_WIDTH << 6) | 20;
    WaitBlitter();

    /* BITPLANE 2 */
    /* CARRY */
    custom->bltapt = H_LINE[2];
    custom->bltbpt = planes[2] + pos;
    custom->bltcpt = carry[1];
    custom->bltdpt = carry[0];

    custom->bltcon0 = FULL_ADDER_CARRY;

    custom->bltsize = (H_LINE_WIDTH << 6) | 20;
    WaitBlitter();
    /* SUM */
    custom->bltapt = H_LINE[2];
    custom->bltbpt = planes[2] + pos;
    custom->bltcpt = &carry[1];
    custom->bltdpt = planes[2] + pos;

    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | (A_OR_B | A_OR_C); // FULL_ADDER;

    custom->bltsize = (H_LINE_WIDTH << 6) | 20;
    WaitBlitter();

    /* PROPAGATE CARRY */
    custom->bltamod = 0;

    custom->bltapt = carry[0];
    custom->bltbpt = planes[0] + pos;
    custom->bltdpt = planes[0] + pos;

    custom->bltcon0 = (SRCA | SRCB | DEST) | A_OR_B;

    custom->bltsize = (H_LINE_WIDTH << 6) | 20;
    WaitBlitter();

    custom->bltapt = carry[0];
    custom->bltbpt = planes[1] + pos;
    custom->bltdpt = planes[1] + pos;

    custom->bltcon0 = (SRCA | SRCB | DEST) | A_OR_B;

    custom->bltsize = (H_LINE_WIDTH << 6) | 20;
    WaitBlitter();
  }
}

static void CalculateFirstLine(void **planes) {
  short word, offset, aux;
  short *ptr;
  short i, j;
  (void)aux;

  /* Set first line to 0 */
  for (i = 0; i < 3; ++i) {
    ptr = planes[i];
    for (j = 0; j < 20; ++j) {
      ptr[j] = 0;
    }
  }

  /* Calculate coordinates */
  word = v_lines[0]/16;
  offset = v_lines[0] - (word * 16);

  /* Draw first beam */
  for (i = 0; i < 3; ++i) {
    ptr = planes[i];
    ptr[word] = V_LINE[i] >> offset;
    if (offset > 2) {
      ptr[word+1] = V_LINE[i] << (16 - offset);
    }
  }

  /* Add rest of the beams */
  for (i = 1; i < NO_OF_LINES; ++i) {
    u_short w1, w2, c1, c2 = 0;

    word = v_lines[i]/16;
    offset = v_lines[i] - (word * 16);

    ptr = planes[0];
    w1 = V_LINE[0] >> offset;
    c1 = ptr[word] & w1;
    ptr[word] = ptr[word] ^ w1;
    if (offset > (16 - V_LINE_WIDTH)) {
      w2 = V_LINE[0] << (16 - offset);
      c2 = ptr[word+1] & w2;
      ptr[word+1] = ptr[word+1] ^ w2;
    }

    ptr = planes[1];
    w1 = V_LINE[1] >> offset;
    /* circural dependency */
    aux = ptr[word];
    ptr[word] = (ptr[word] ^ w1) ^ c1;
    c1 = ((aux ^ w1) & c1) ^ (aux & w1);
    if (offset > (16 - V_LINE_WIDTH)) {
      w2 = V_LINE[1] << (16 - offset);
      aux = ptr[word+1];
      ptr[word+1] = ptr[word+1] ^ w2 ^ c2;
      c2 = ((aux ^ w2) & c2) ^ (aux & w2);
    }

    ptr = planes[2];
    w1 = V_LINE[2] >> offset;
    aux = ptr[word];
    ptr[word] = ptr[word] ^ w1 ^ c1;
    c1 = ((aux ^ w1) & c1) ^ (aux & w1);
    if (offset > (16 - V_LINE_WIDTH)) {
      w2 = V_LINE[2] << (16 - offset);
      aux = ptr[word+1];
      ptr[word+1] = ptr[word+1] ^ w2 ^ c2;
      c2 = ((aux ^ w2) & c2) ^ (aux & w2);
    }
    
    ptr[word] |= c1;
    ptr[word+1] |= c2;
    ptr = planes[0];
    ptr[word] |= c1;
    ptr[word+1] |= c2;
    ptr = planes[1];
    ptr[word] |= c1;
    ptr[word+1] |= c2;
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

static void Move(void) {
  short i = 0;

  if (frameCount % 3 == 0) {
    return;
  }
  h_lines[0] += 1;
  h_lines[1] += 2;
  h_lines[2] += 1;
  h_lines[3] -= 1;
  h_lines[4] -= 2;
  h_lines[5] -= 1;

  v_lines[0] += 1;
  v_lines[1] += 2;
  v_lines[2] += 1;
  v_lines[3] -= 1;
  v_lines[4] -= 2;
  v_lines[5] -= 1;

  for (i = 0; i < NO_OF_LINES; ++i) {
    if (h_lines[i] > HEIGHT - 7) {
      h_lines[i] = 0;
    }
    if (h_lines[i] < 0) {
      h_lines[i] = HEIGHT - 7;
    }
  }

  for (i = 0; i < NO_OF_LINES; ++i) {
    if (v_lines[i] > 306) {
      v_lines[i] = 0;
    }
    if (v_lines[i] < 0) {
      v_lines[i] = 306;
    }
  }
}


static CopListT *MakeCopperList(void) {
  short i = 0;
  (void)i;

  cp = NewCopList(1024);
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);

  /* Line duplication */
  // for (i = 0; i < HEIGHT * 2; i++) {
  //   CopWaitSafe(cp, Y(i), 0);
  //   CopMove16(cp, bpl1mod, ((i & 1) != 1) ? -40 : 0);
  //   CopMove16(cp, bpl2mod, ((i & 1) != 1) ? -40 : 0);
  // }

  CopListFinish(cp);

  return cp;
}

static void Init(void) {
  EnableDMA(DMAF_BLITTER | DMAF_BLITHOG | DMAF_RASTER);

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, 256);

  // SetColor(0, 0x000);
  // SetColor(1, 0x313);
  // SetColor(2, 0x535);
  // SetColor(3, 0x757);
  // SetColor(4, 0x979);
  // SetColor(5, 0xB9B);
  // SetColor(6, 0xDBD);
  // SetColor(7, 0xFFF);

  SetColor(0, 0x000);
  SetColor(1, 0x313);
  SetColor(2, 0x535);
  SetColor(3, 0x757);
  SetColor(4, 0x979);
  SetColor(5, 0xC9C);
  SetColor(6, 0xFBF);
  SetColor(7, 0xFFF);

  cp = MakeCopperList();
  CopListActivate(cp);
}

static void Kill(void) {
  DisableDMA(DMAF_BLITTER | DMAF_BLITHOG | DMAF_RASTER);

  DeleteCopList(cp);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

PROFILE(Total);

static void Render(void) {
  ProfilerStart(Total);
  {
    CalculateFirstLine(screen[active]->planes); // 20
    VerticalFill(screen[active]->planes); // 215
    DoMagic(screen[active]->planes); // 149
    Move();
  }
  ProfilerStop(Total);

  ITER(i, 0, DEPTH - 1, CopInsSet32(&bplptr[i], screen[active]->planes[i]));

  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Darkroom, NULL, NULL, Init, Kill, Render, NULL);