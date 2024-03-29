#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "fade.h"


#define DMG_FADE_STEP_COUNT (4u)

// Each array entry is 1 x uint8_t
const uint8_t fade_steps_dmg_BGP_board[] = {
    DMG_PALETTE(DMG_WHITE,     DMG_BLACK, DMG_DARK_GRAY, DMG_LITE_GRAY),
    DMG_PALETTE(DMG_LITE_GRAY, DMG_BLACK, DMG_BLACK,     DMG_DARK_GRAY),
    DMG_PALETTE(DMG_DARK_GRAY, DMG_BLACK, DMG_BLACK,     DMG_BLACK),
    DMG_PALETTE(DMG_BLACK,     DMG_BLACK, DMG_BLACK,     DMG_BLACK)
};

const uint8_t fade_steps_dmg_BGP_title[] = {
    DMG_PALETTE(DMG_WHITE,     DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK),
    DMG_PALETTE(DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK,     DMG_BLACK),
    DMG_PALETTE(DMG_DARK_GRAY, DMG_BLACK,     DMG_BLACK,     DMG_BLACK),
    DMG_PALETTE(DMG_BLACK,     DMG_BLACK,     DMG_BLACK,     DMG_BLACK)
};

const uint8_t fade_steps_dmg_OBP0[] = {
    DMG_PALETTE(DMG_WHITE,     DMG_WHITE,     DMG_BLACK,     DMG_BLACK),
    DMG_PALETTE(DMG_LITE_GRAY, DMG_LITE_GRAY, DMG_BLACK,     DMG_BLACK),
    DMG_PALETTE(DMG_DARK_GRAY, DMG_DARK_GRAY, DMG_BLACK,     DMG_BLACK),
    DMG_PALETTE(DMG_BLACK,     DMG_BLACK,     DMG_BLACK,     DMG_BLACK)
};

// Not currently used
const uint8_t fade_steps_dmg_OBP1[] = {
    DMG_PALETTE(DMG_WHITE,     DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK),
    DMG_PALETTE(DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK,     DMG_BLACK),
    DMG_PALETTE(DMG_DARK_GRAY, DMG_BLACK,     DMG_BLACK,     DMG_BLACK),
    DMG_PALETTE(DMG_BLACK,     DMG_BLACK,     DMG_BLACK,     DMG_BLACK)
};


// Fade out to black
void fade_out(uint8_t delay_len, uint8_t which_bg_pal) {
    for (uint8_t c = 0; c < DMG_FADE_STEP_COUNT; c++) {
        vsync();
        if (which_bg_pal == BG_PAL_TITLE)
            BGP_REG  = fade_steps_dmg_BGP_title[c];
        else
            BGP_REG  = fade_steps_dmg_BGP_board[c];
        OBP0_REG = fade_steps_dmg_OBP0[c];
        OBP1_REG = fade_steps_dmg_OBP1[c];
        if (delay_len) delay(delay_len);
    }
}


// Fade in from black
void fade_in(uint8_t delay_len, uint8_t which_bg_pal) {

    for (uint8_t c = 0; c < DMG_FADE_STEP_COUNT; c++) {
        vsync();
        if (which_bg_pal == BG_PAL_TITLE)
            BGP_REG  = fade_steps_dmg_BGP_title[ARRAY_LEN(fade_steps_dmg_BGP_title) - 1 - c];
        else
            BGP_REG  = fade_steps_dmg_BGP_board[ARRAY_LEN(fade_steps_dmg_BGP_board) - 1 - c];
        OBP0_REG = fade_steps_dmg_OBP0[ARRAY_LEN(fade_steps_dmg_OBP0) - 1 - c];
        OBP1_REG = fade_steps_dmg_OBP1[ARRAY_LEN(fade_steps_dmg_OBP1) - 1 - c];
        if (delay_len) delay(delay_len);
    }
}
