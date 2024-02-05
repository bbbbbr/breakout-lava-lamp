#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "gameboard.h"


// Main game state (copied to cart for  save and restore)
gameinfo_t gameinfo;


// Background colors for the board (2x1 and 2x2 style)
const uint8_t board_player_colors[PLAYER_COUNT_MAX] = {
    BOARD_COL_WHITE,  BOARD_COL_BLACK,
    BOARD_COL_D_GREY, BOARD_COL_L_GREY
};

// Sprite colors for the player ball (2x1 and 2x2 style)
const uint8_t player_colors[PLAYER_COUNT_MAX] = {
    PLAYER_COL_BLACK, PLAYER_COL_WHITE,
    PLAYER_COL_WHITE, PLAYER_COL_BLACK
};