#include "custom.h"
#include <debug.h>
#include <copper.h>

void CopListStop(void) {
  /* Disable copper DMA */
  DisableDMA(DMAF_COPPER);
}
