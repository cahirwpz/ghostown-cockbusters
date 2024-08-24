#include <custom.h>
#include <effect.h>
#include <color.h>
#include <palette.h>
#include <sync.h>
#include <sprite.h>
#include <system/task.h>
#include <system/interrupt.h>
#include <system/amigahunk.h>
#include <system/file.h>

#include "demo.h"

#define _SYSTEM
#include <system/boot.h>
#include <system/cia.h>
#include <system/floppy.h>
#include <system/filesys.h>
#include <system/memfile.h>
#include <system/memory.h>

short frameFromStart;
short frameTillEnd;

#include "data/demo.c"

static void ShowMemStats(void) {
  Log("[Memory] CHIP: %d FAST: %d\n", MemAvail(MEMF_CHIP), MemAvail(MEMF_FAST));
}

#define EXE_LOADER 0
#define EXE_DNA3D 1
#define EXE_STENCIL3D 2
#define EXE_COCK_FOLK 3
#define EXE_TEXOBJ 4
#define EXE_FLOWER3D 5
#define EXE_LOGO 6
#define EXE_ROTATOR 7
#define EXE_TEXTSCROLL 8
#define EXE_ABDUCTION 9
#define EXE_COCK_TECHNO 10
#define EXE_LOGO_GTN 11
#define EXE_PROTRACKER 12
#define EXE_LAST 13

typedef struct ExeFile {
  const char *path;
  HunkT *hunk;
  EffectT *effect;
} ExeFileT;

#define EXEFILE(NUM, PATH) [NUM] = { .path = PATH, .hunk = NULL, .effect = NULL }

static __code ExeFileT ExeFile[EXE_LAST] = {
  EXEFILE(EXE_LOADER, "loader.exe"),
  EXEFILE(EXE_DNA3D, "dna3d.exe"),
  EXEFILE(EXE_STENCIL3D, "stencil3d.exe"),
  EXEFILE(EXE_COCK_FOLK, "cock-folk.exe"),
  EXEFILE(EXE_TEXOBJ, "texobj.exe"),
  EXEFILE(EXE_FLOWER3D, "flower3d.exe"),
  EXEFILE(EXE_LOGO, "logo.exe"),
  EXEFILE(EXE_ROTATOR, "rotator.exe"),
  EXEFILE(EXE_TEXTSCROLL, "textscroll.exe"),
  EXEFILE(EXE_ABDUCTION, "abduction.exe"),
  EXEFILE(EXE_COCK_TECHNO, "cock-techno.exe"),
  EXEFILE(EXE_LOGO_GTN, "logo_gtn.exe"),
  EXEFILE(EXE_PROTRACKER, "playpt.exe"),
};

static EffectT *LoadExe(int num) {
  ExeFileT *exe = &ExeFile[num];
  FileT *file;
  HunkT *hunk;
  u_char *ptr;

  Log("[Effect] Downloading '%s'\n", exe->path);

  file = OpenFile(exe->path);
  hunk = LoadHunkList(file);
  /* Assume code section is first and effect definition is at its end.
   * That should be the case as the effect definition is always the last in
   * source file. */
  ptr = &hunk->data[hunk->size - sizeof(EffectT)];
  while ((u_char *)ptr >= hunk->data) {
    if (*(u_int *)ptr == EFFECT_MAGIC) {
      EffectT *effect = (EffectT *)ptr;
      Log("[Effect] Found '%s'\n", effect->name);
      exe->effect = effect;
      exe->hunk = hunk;
      EffectLoad(effect);
      return effect;
    }
    ptr -= 2;
  }
  Log("%s: missing effect magic marker\n", exe->path);
  PANIC();
  return NULL;
}

static void UnLoadExe(int num) {
  ExeFileT *exe = &ExeFile[num];

  Log("[Effect] Removing '%s'\n", exe->path);

  EffectUnLoad(exe->effect);
  exe->effect = NULL;
  FreeHunkList(exe->hunk);
  exe->hunk = NULL;
  ShowMemStats();
}

void FadeBlack(const u_short *colors, short count, u_int start, short step) {
  volatile u_short *reg = &custom->color[start];
  
  if (step < 0)
    step = 0;
  if (step > 15)
    step = 15;

  while (--count >= 0) {
    short to = *colors++;

    short r = ((to >> 4) & 0xf0) | step;
    short g = (to & 0xf0) | step;
    short b = ((to << 4) & 0xf0) | step;

    r = colortab[r];
    g = colortab[g];
    b = colortab[b];
    
    *reg++ = (r << 4) | g | (b >> 4);
  }
}

short UpdateFrameCount(void) {
  short t = ReadFrameCounter();
  frameCount = t;
  frameFromStart = t - CurrKeyFrame(&EffectNumber);
  frameTillEnd = NextKeyFrame(&EffectNumber) - t;
  return t;
}

static volatile EffectFuncT VBlankHandler = NULL;

static int VBlankISR(void) {
  if (VBlankHandler)
    VBlankHandler();
  return 0;
}

INTSERVER(VBlankInterrupt, 0, (IntFuncT)VBlankISR, NULL);

typedef enum {
  BG_IDLE = 0,
  BG_INIT = 1,
  BG_DEMO = 2,
} BgTaskStateT;

static __code volatile BgTaskStateT BgTaskState = BG_IDLE;

static void BgTaskLoop(__unused void *ptr) {
  Log("[BgTask] Started!\n");

  for (;;) {
    switch (BgTaskState) {
      case BG_INIT:
        LoadExe(EXE_PROTRACKER);
        LoadExe(EXE_LOGO_GTN);
        LoadExe(EXE_FLOWER3D);
        LoadExe(EXE_COCK_FOLK);
        LoadExe(EXE_ABDUCTION);
        LoadExe(EXE_DNA3D);
        LoadExe(EXE_LOGO);

        Log("[BgTask] Done initial loading!\n");
        BgTaskState = BG_IDLE;
        break;

      case BG_DEMO:
        {
          short cmd = TrackValueGet(&EffectLoader, ReadFrameCounter());
          if (cmd == 0)
            continue;

          Log("[BgTask] Got command %d!\n", cmd);

          if (cmd > 0) {
            /* load effect */
            LoadExe(cmd);
          } else {
            /* unload effect */
            cmd = -cmd;

            while (EffectIsRunning(ExeFile[cmd].effect));

            UnLoadExe(cmd);
          }
        }
        break;

      default:
        break;
    }
  }
}

static __aligned(8) char BgTaskStack[512];
static TaskT BgTask;

static void RunLoader(void) {
  EffectT *Loader = LoadExe(EXE_LOADER);

  EffectInit(Loader);
  VBlankHandler = Loader->VBlank;

  SetFrameCounter(0);
  lastFrameCount = 0;
  frameCount = 0;
  BgTaskState = BG_INIT;

  while (BgTaskState != BG_IDLE) {
    short t = UpdateFrameCount();
    if (lastFrameCount != frameCount) {
      Loader->Render();
      TaskWaitVBlank();
    }
    lastFrameCount = t;
  }

  EffectKill(Loader);
  UnLoadExe(EXE_LOADER);
}

static void RunEffects(void) {
  SetFrameCounter(0);
  lastFrameCount = 0;
  frameCount = 0;
  BgTaskState = BG_DEMO;

  for (;;) {
    static short prev = -1;
    short curr = TrackValueGet(&EffectNumber, frameCount);

    // Log("prev: %d, curr: %d, frameCount: %d\n", prev, curr, frameCount);

    if (prev != curr) {
      if (prev >= 0) {
        VBlankHandler = NULL;
        EffectKill(ExeFile[prev].effect);
        ShowMemStats();
      }
      if (curr == -1)
        break;
      EffectInit(ExeFile[curr].effect);
      VBlankHandler = ExeFile[curr].effect->VBlank;
      ShowMemStats();
      Log("[Effect] Transition to %s took %d frames!\n",
          ExeFile[curr].effect->name, ReadFrameCounter() - lastFrameCount);
      lastFrameCount = ReadFrameCounter() - 1;
    }

    {
      EffectT *effect = ExeFile[curr].effect;
      short t = UpdateFrameCount();
      if ((lastFrameCount != frameCount) && effect->Render)
        effect->Render();
      lastFrameCount = t;
    }

    prev = curr;
  }
}

#define ROMADDR 0xf80000
#define ROMSIZE 0x07fff0
#define ROMEXTADDR 0xe00000
#define ROMEXTSIZE 0x080000

static const MemBlockT rom[] = {
  {(const void *)ROMADDR, ROMSIZE},
  {(const void *)ROMEXTADDR, ROMEXTSIZE},
  {NULL, 0}
};

int main(void) {
  /* NOP that triggers fs-uae debugger to stop and inform GDB that it should
   * fetch segments locations to relocate symbol information read from file. */
  asm volatile("exg %d7,%d7");

  {
    FileT *dev = NULL;

    if (BootDev == 0) /* floppy */ {
        dev = FloppyOpen();
    } else if (BootDev == 1) /* rom/baremetal */ {
        dev = MemOpen(rom);
    } else {
        PANIC();
    }

    InitFileSys(dev);
  }

  ResetSprites();
  AddIntServer(INTB_VERTB, VBlankInterrupt);

  /* Background thread may use tracks as well. */
  {
    TrackT **trkp = AllTracks;
    while (*trkp)
      TrackInit(*trkp++);
  }

  TaskInit(&BgTask, "background", BgTaskStack, sizeof(BgTaskStack));
  TaskRun(&BgTask, 1, BgTaskLoop, NULL);

  RunLoader();

  EffectInit(ExeFile[EXE_PROTRACKER].effect);
  RunEffects();
  EffectKill(ExeFile[EXE_PROTRACKER].effect);

  RemIntServer(INTB_VERTB, VBlankInterrupt);
  KillFileSys();

  return 0;
}
