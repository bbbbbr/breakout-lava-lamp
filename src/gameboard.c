#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
// #include <gbdk/emu_debug.h>
#include <rand.h>

#include "common.h"
#include "input.h"
#include "gfx.h"
#include "math_util.h"
#include "save_and_restore.h"

#include "gameboard.h"
#include "players.h"


// Initialize the game board grid to 2 or 4 regions
static void board_init_grid(void) {

    uint8_t * p_board = gameinfo.board;
    for (uint8_t y = 0; y < BOARD_DISP_H; y++) {
        for (uint8_t x = 0; x < BOARD_DISP_W; x++) {

            if (gameinfo.player_count == PLAYERS_2_VAL) {

                // Divide board into 2 team color regions, Left & Right
                *p_board =  x / (BOARD_DISP_W / 2); // x + (y * BOARD_DISP_W);
                // Old method, look up color based on team (now just use directly)
                // *p_board = board_team_bg_colors[ x / (BOARD_DISP_W / 2) ]; // x + (y * BOARD_DISP_W);
            }
            else { // Default, 2x2 grid split

                // Divide board into 4 team color regions, Left & Right and Top & Bottom
                *p_board = (x / (BOARD_DISP_W / 2)) + ((y / (BOARD_DISP_H / 2)) * 2u);
                // Old method, look up color based on team (now just use directly)
                // *p_board = board_team_bg_colors[ (x / (BOARD_DISP_W / 2)) + ((y / (BOARD_DISP_H / 2)) * 2u) ];
            }
            p_board++;
        }
        // Handle rowstride difference between display and full buffer sizes
        p_board += BOARD_BUF_W - BOARD_DISP_W;
    }
}


// Expects screen faded out
void board_init_gfx(void) {
    // Palette loading is handled by the fade routine

    // Load the tiles
    set_bkg_data(0,    bg_tile_patterns_length  / TILE_PATTERN_SZ, bg_tile_patterns);

    set_sprite_data(0, spr_tile_patterns_length / TILE_PATTERN_SZ, spr_tile_patterns);

    // Draw the board
    set_bkg_tiles(0u, 0u, BOARD_BUF_W, BOARD_BUF_H, gameinfo.board);

    hide_sprites_range(0u, MAX_HARDWARE_SPRITES - 1);

    for (uint8_t c = 0; c < PLAYER_COUNT_MAX; c++) {
        set_sprite_tile(c, player_sprite_colors[c & PLAYER_TEAMS_MASK]);  // Mask down player id to one of the teams
    }

    if (gameinfo.sprites_enabled) players_redraw_sprites();
}


// Reset Board and Players states
// Not called when "Continue" Title Menu action is used, unless the number of players was changed
void board_reset(void) {

    if (gameinfo.action == ACTION_RANDOM)
        initrand(gameinfo.user_rand_seed.w);
    else
        initrand(RAND_SEED_STANDARD);

    players_reset();
    players_all_recalc_movement();
    board_init_grid();
}

