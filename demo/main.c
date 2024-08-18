#include <custom.h>
#include <effect.h>
#include <ptplayer.h>
#include <color.h>
#include <copper.h>
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
#include <system/memory.h>
#include <system/cia.h>
#include <system/floppy.h>
#include <system/filesys.h>
#include <system/memfile.h>
#include <system/file.h>
#include <system/memory.h>
#include <system/interrupt.h>

short frameFromStart;
short frameTillEnd;

#include "data/demo.c"

#define EFF_DNA3D 0
#define EFF_COCKSTENCIL 1
#define EFF_ANIMCOCK 2
#define EFF_TEXOBJ 3

static __code EffectT *AllEffects[] = {
  [EFF_DNA3D] = NULL,
  [EFF_COCKSTENCIL] = NULL,
  [EFF_ANIMCOCK] = NULL,
  [EFF_TEXOBJ] = NULL,
};

static __code EffectT *Loader;
static __code EffectT *Protracker;

static __code HunkT *LoaderExe;
static __code HunkT *ProtrackerExe;
static __code HunkT *Dna3DExe;
static __code HunkT *CockStencilExe;
static __code HunkT *AnimCockExe;
static __code HunkT *TexObjExe;

static HunkT *LoadExe(const char *path, EffectT **effect_p) {
  FileT *file;
  HunkT *hunk;
  u_char *ptr;

  Log("[Effect] Downloading '%s'\n", path);

  file = OpenFile(path);
  hunk = LoadHunkList(file);
  /* Assume code section is first and effect definition is at its end.
   * That should be the case as the effect definition is always the last in
   * source file. */
  ptr = &hunk->data[hunk->size - sizeof(EffectT)];
  while ((u_char *)ptr >= hunk->data) {
    if (*(u_int *)ptr == EFFECT_MAGIC) {
      EffectT *effect = (EffectT *)ptr;
      Log("[Effect] Found '%s'\n", effect->name);
      EffectLoad(effect);
      *effect_p = effect;
      return hunk;
    }
    ptr -= 2;
  }
  Log("%s: missing effect magic marker\n", path);
  PANIC();
  return NULL;
}

static void LoadDemo(void) {
  LoaderExe = LoadExe("loader.exe", &Loader);
  frameCount = 0;

  EffectInit(Loader);

  ProtrackerExe = LoadExe("playpt.exe", &Protracker);
  frameCount = 64;
  Loader->Render();

  Dna3DExe = LoadExe("dna3d.exe", &AllEffects[EFF_DNA3D]);
  frameCount = 96;
  Loader->Render();

  CockStencilExe = LoadExe("stencil3d.exe", &AllEffects[EFF_COCKSTENCIL]);
  frameCount = 128;
  Loader->Render();

  AnimCockExe = LoadExe("anim-polygons.exe", &AllEffects[EFF_ANIMCOCK]);
  frameCount = 160;
  Loader->Render();

  TexObjExe = LoadExe("texobj.exe", &AllEffects[EFF_TEXOBJ]);
  frameCount = 192;
  Loader->Render();

  EffectKill(Loader);
  EffectUnLoad(Loader);
  FreeHunkList(LoaderExe);
}

static void ShowMemStats(void) {
  Log("[Memory] CHIP: %d FAST: %d\n", MemAvail(MEMF_CHIP), MemAvail(MEMF_FAST));
}

void FadeBlack(const u_short *colors, short count, u_int start, short step) {
  volatile short *reg = &custom->color[start];
  
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

static void RunEffects(void) {
  AddIntServer(INTB_VERTB, VBlankInterrupt);

  for (;;) {
    static short prev = -1;
    short curr = TrackValueGet(&EffectNumber, frameCount);

    // Log("prev: %d, curr: %d, frameCount: %d\n", prev, curr, frameCount);

    if (prev != curr) {
      if (prev >= 0) {
        VBlankHandler = NULL;
        EffectKill(AllEffects[prev]);
        ShowMemStats();
      }
      if (curr == -1)
        break;
      EffectInit(AllEffects[curr]);
      VBlankHandler = AllEffects[curr]->VBlank;
      ShowMemStats();
      Log("[Effect] Transition to %s took %d frames!\n",
          AllEffects[curr]->name, ReadFrameCounter() - lastFrameCount);
      lastFrameCount = ReadFrameCounter() - 1;
    }

    {
      EffectT *effect = AllEffects[curr];
      short t = UpdateFrameCount();
      if ((lastFrameCount != frameCount) && effect->Render)
        effect->Render();
      lastFrameCount = t;
    }

    prev = curr;
  }

  RemIntServer(INTB_VERTB, VBlankInterrupt);
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
  LoadDemo();

  {
    TrackT **trkp = AllTracks;
    while (*trkp)
      TrackInit(*trkp++);
  }

  EffectInit(Protracker);
  RunEffects();
  EffectKill(Protracker);

  KillFileSys();

  return 0;
}
