#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "gameboard.h"


// Main game state (copied to cart for  save and restore)
gameinfo_t gameinfo;

uint8_t vsync_count;


// Sprite colors for the player ball (2x1 and 2x2 style)
// Should be opposite of intensity to board colors
const uint8_t player_sprite_colors[PLAYER_TEAMS_COUNT] = {
    PLAYER_SPR_COL_BLACK, PLAYER_SPR_COL_WHITE,
    PLAYER_SPR_COL_WHITE, PLAYER_SPR_COL_BLACK
};


// Board and player grid distribution patterns
// Ball count 2,4,8,16,32
const uint8_t players_divs_x[] = {2u,  2u,  4u,  4u,  8u };
const uint8_t players_divs_y[] = {1u,  2u,  2u,  4u,  4u };

const uint8_t board_divs_x[] = {2u,  2u,  4u,  4u,  8u };
const uint8_t board_divs_y[] = {1u,  2u,  2u,  4u,  4u };
