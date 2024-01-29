#ifndef _FADE_H
#define _FADE_H

#include "common.h"

#define FADE_DELAY_NORM       (70u)
#define FADE_DELAY_HELP_IN    (40u)   // Faster fade while FX are running
#define FADE_DELAY_HELP_OUT   (120u)
#define FADE_DELAY_INTRO      (40u)  // Little faster than usual

void fade_in(uint8_t delay_len);
void fade_out(uint8_t delay_len);

#endif // _FADE_H
