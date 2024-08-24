#include <custom.h>
#include <copper.h>

static __data_chip __aligned(2) u_int NullCopperList = 0xfffffffe;

void CopperStop(void) {
  /* Load empty list and instruct copper to jump to them immediately. */
  custom->cop1lc = (u_int)&NullCopperList;
  custom->cop2lc = (u_int)&NullCopperList;
  (void)custom->copjmp1;
  (void)custom->copjmp2;
  /* Disable copper DMA and all systems that rely on frame-by-frame refresh
   * by Copper. */
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_SPRITE);
}
