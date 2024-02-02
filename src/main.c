#include <gbdk/platform.h>
#include <stdint.h>
#include <rand.h>

#include "common.h"
#include "input.h"
#include "fade.h"

#include "gameboard.h"

#define MBC_RAM_BANK_0    0u  // default to RAM Bank 0

static void main_init(void) {
    // #if defined(CART_mbc5) || defined(CART_mbc5_rumble)
        // Initialize MBC bank defaults
        // Upper ROM bank to 1, And SRAM/XRAM bank to 0
        SWITCH_ROM_MBC5(1);
        SWITCH_RAM(MBC_RAM_BANK_0);
        DISABLE_RAM_MBC5;
    // #endif

    HIDE_BKG;
    HIDE_SPRITES;
    DISPLAY_ON;
	UPDATE_KEYS();

    fade_out(FADE_DELAY_NORM);
    SHOW_BKG;
    SHOW_SPRITES;
}

void main(void){

    main_init();
    gameinfo.state = STATE_RUNGAME;
    gameinfo.player_count = PLAYER_COUNT_DEFAULT;

    // TODO: wait for non-deterministic user input to seed random numbers
    initrand(0x1234u);

    while (TRUE) {
        switch(gameinfo.state) {

            // case STATE_STARTUP:
            //     break;

            // case STATE_SHOWSPLASH:
            //     break;

            // case STATE_SHOWTITLE:
            //     // ...
            //     fade_in(FADE_DELAY_NORM);
            //     // ...
            //     fade_out(FADE_DELAY_NORM);
            //     gameinfo.state = STATE_RUNGAME;
            //     break;

            case STATE_RUNGAME:
                board_init();
                fade_in(FADE_DELAY_NORM);

                board_run();
                fade_out(FADE_DELAY_NORM);
                gameinfo.state = STATE_RUNGAME;
                break;

            // case STATE_GAMEDONE:
            //     break;
        }
        vsync();
    }
}
