#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "gameboard.h"


// Main game state (copied to cart for  save and restore)
gameinfo_t gameinfo;

uint8_t vsync_count;


// Old method, look up color based on team (now just use player id directly)
//
// // Background colors for the board (2x1 and 2x2 style)
// const uint8_t board_team_bg_colors[PLAYER_TEAMS_COUNT] = {
//     BOARD_COL_WHITE,  BOARD_COL_BLACK,
//     BOARD_COL_D_GREY, BOARD_COL_L_GREY
// };

// Sprite colors for the player ball (2x1 and 2x2 style)
// Should be opposite of intensity to board colors
const uint8_t player_sprite_colors[PLAYER_TEAMS_COUNT] = {
    PLAYER_SPR_COL_BLACK, PLAYER_SPR_COL_WHITE,
    PLAYER_SPR_COL_WHITE, PLAYER_SPR_COL_BLACK
};