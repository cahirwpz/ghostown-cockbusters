#include <effect.h>

#include <system/interrupt.h> /* register & unregister an interrupt handler */

static void Render(void) {
  TaskWaitVBlank();
}

EFFECT(Empty, NULL, NULL, NULL, NULL, Render, NULL);
