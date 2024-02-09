#ifndef _FADE_H
#define _FADE_H

#include "common.h"

#define FADE_DELAY_NORM       (70u)
#define FADE_DELAY_HELP_IN    (40u)   // Faster fade while FX are running
#define FADE_DELAY_HELP_OUT   (120u)
#define FADE_DELAY_INTRO      (40u)  // Little faster than usual

#define BG_PAL_TITLE 0
#define BG_PAL_BOARD 1

void fade_in(uint8_t delay_len, uint8_t which_bg_pal);
void fade_out(uint8_t delay_len, uint8_t which_bg_pal);

#endif // _FADE_H
