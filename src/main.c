#include <gbdk/platform.h>
#include <stdint.h>
#include <rand.h>

#include "common.h"
#include "input.h"
#include "fade.h"

#include "gameboard.h"
#include "title_screen.h"

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

    // TODO: move this to save-loading
    gameinfo.state = STATE_SHOWTITLE;
    gameinfo.is_initialized = false;

    gameinfo.action       = ACTION_CONTINUE_VAL;
    gameinfo.speed        = SPEED_SLOW_VAL;
    gameinfo.player_count = PLAYERS_4_VAL;
    gameinfo.player_count_last = gameinfo.player_count;

    while (TRUE) {
        switch(gameinfo.state) {

            // case STATE_SHOWSPLASH:
            //     break;

            case STATE_SHOWTITLE:
                title_init();
                fade_in(FADE_DELAY_NORM);

                title_run();
                fade_out(FADE_DELAY_NORM);
                gameinfo.state = STATE_RUNGAME;
                break;

            case STATE_RUNGAME:
                // Reset the board if needed or the user is not Continuing
                if ((gameinfo.is_initialized == false) || (gameinfo.action != ACTION_CONTINUE_VAL)) {
                    gameinfo.is_initialized = true;
                    board_reset();
                }

                board_init();
                fade_in(FADE_DELAY_NORM);

                board_run();
                fade_out(FADE_DELAY_NORM);
                gameinfo.state = STATE_SHOWTITLE;
                break;
        }
        vsync();
    }
}
