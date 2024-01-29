#include <gbdk/platform.h>
#include <stdint.h>
#include <rand.h>

#include "common.h"
#include "input.h"
#include "fade.h"

#include "gameboard.h"


void init(void) {
    DISPLAY_OFF;
	UPDATE_KEYS();

    fade_out(FADE_DELAY_NORM);
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}


void main(void){
    init();
    gameinfo.state = STATE_RUNGAME;
    gameinfo.player_count = PLAYER_COUNT_4;
    // gameinfo.player_count = PLAYER_COUNT_DEFAULT;

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
