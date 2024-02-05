#include <gbdk/platform.h>
#include <stdint.h>
#include <rand.h>

#include "common.h"
#include "input.h"
#include "fade.h"

#include "save_and_restore.h"
#include "gameboard.h"
#include "gameplay.h"
#include "title_screen.h"

#define MBC_RAM_BANK_0    0u  // default to RAM Bank 0

uint8_t gamestate; // main loop state

static void main_init(void) {
    #if defined(CART_mbc5) || defined(CART_mbc5_rumble)
        // Initialize MBC bank defaults
        // Upper ROM bank to 1, And SRAM/XRAM bank to 0
        SWITCH_ROM_MBC5(1);
        SWITCH_RAM(MBC_RAM_BANK_0);
        DISABLE_RAM_MBC5;
    #endif

    HIDE_BKG;
    HIDE_SPRITES;
    DISPLAY_ON;
	UPDATE_KEYS();

    fade_out(FADE_DELAY_NORM);
    SHOW_BKG;
    SHOW_SPRITES;

    savedata_load();
}

void main(void){

    main_init();

    gamestate = STATE_SHOWTITLE;

    while (TRUE) {
        switch(gamestate) {

            // case STATE_SHOWSPLASH:
            //     break;

            case STATE_SHOWTITLE:
                title_init();
                fade_in(FADE_DELAY_NORM);

                title_run();
                fade_out(FADE_DELAY_NORM);
                gamestate = STATE_RUNGAME;
                break;

            case STATE_RUNGAME:
                game_init();
                fade_in(FADE_DELAY_NORM);

                game_run();
                fade_out(FADE_DELAY_NORM);
                gamestate = STATE_SHOWTITLE;
                break;
        }
        vsync();
    }
}
