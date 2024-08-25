#ifndef __DEMO_H__
#define __DEMO_H__

#include <bitmap.h>
#include <palette.h>
#include <sync.h>
#include <effect.h>

short ReadFrameCount(void);

void FadeBlack(const u_short *colors, short count, u_int start, short step);

#endif /* !__DEMO_H__ */
