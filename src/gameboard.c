#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
// #include <gbdk/emu_debug.h>
#include <rand.h>

#include "common.h"
#include "input.h"
#include "gfx.h"
#include "math_util.h"
#include "save_and_restore.h"

#include "title_screen.h"
#include "gameboard.h"
#include "players.h"


void board_set_tile(int8_t board_x, int8_t board_y, uint8_t team) {
    if ((uint8_t)board_x > BOARD_DISP_W) return;
    if ((uint8_t)board_y > BOARD_DISP_H) return;

    gameinfo.board[ BOARD_INDEX( ((uint16_t)board_x), ((uint16_t)board_y) ) ] = team;
}


// Initialize the game board as a grid always divided to match the number of players
// Pairs with: players_reset_grid_for_all_sizes()
static void board_init_grid_for_all_sizes(uint8_t player_count_idx) {

    uint8_t * p_board = gameinfo.board;
    uint8_t team_color;

    const uint8_t divs_x = board_divs_x[player_count_idx];
    const uint8_t divs_y = board_divs_y[player_count_idx];

    // Fixed point calc for widths to deal with cases where board width isn't evenly divisible
    const uint16_t step_x = (BOARD_DISP_W << 8) / (uint16_t)divs_x;
    const uint16_t step_y = (BOARD_DISP_H << 8) / (uint16_t)divs_y;

    uint16_t area_x, area_y;

    // Divide the board into NxN regions, fill each region with a given team's color
    for (uint16_t y = 0; y < divs_y; y++) {
        for (uint16_t x = 0; x < divs_x; x++) {

            // Repeating 2x2 grid of 4 different colors
            team_color = (x & 0x01u) + ((y & 0x01u) * 2u);

            for (area_y = (y * step_y); area_y < ((y + 1u) * step_y); area_y += 1u << 8) {
                for (area_x = (x * step_x); area_x < ((x + 1u) * step_x); area_x+= 1u << 8) {

                    gameinfo.board[(area_x >> 8) + ((area_y >> 8) * BOARD_BUF_W)] = team_color; //(team & PLAYER_TEAMS_MASK);
                }
            }
        }
    }
}

/*
// Initialize the game board as grids for player counts <=8, random for larger sizes
static void board_init_grid_small_sizes(void) {
    uint8_t * p_board = gameinfo.board;

    for (uint8_t y = 0; y < BOARD_DISP_H; y++) {
        for (uint8_t x = 0; x < BOARD_DISP_W; x++) {

            if (gameinfo.player_count == PLAYERS_2_VAL) {

                // Divide board into 2 team color regions, Left & Right
                *p_board =  x / (BOARD_DISP_W / 2);
            }
            else {
            // else if (gameinfo.player_count <= PLAYERS_8_VAL) {

                // Divide board into 4 team color regions, Left & Right and Top & Bottom
                *p_board = (x / (BOARD_DISP_W / 2)) + ((y / (BOARD_DISP_H / 2)) * 2u);
            }
            // else { // Default, 2x2 grid split
            //     // Random colors for larger player counts (it looks more pleasing than a huge grid)
            //     *p_board = rand() & PLAYER_TEAMS_MASK;
            // }
            p_board++;
        }
        // Handle rowstride difference between display and full buffer sizes
        p_board += BOARD_BUF_W - BOARD_DISP_W;
    }
}
*/


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



// Reset Board
// Not called when "Continue" Title Menu action is used
void board_reset(void) {

    uint8_t player_count_idx = settings_get_setting_index(MENU_PLAYERS);

    // For alternate grid sizing, cap max grid sizing at 4 player setup
    if (KEY_PRESSED(BUTTON_SELECT_INIT_ALTERNATE) && (player_count_idx > PLAYERS_4_IDX)) {
        player_count_idx = PLAYERS_4_IDX;
    }

    board_init_grid_for_all_sizes(player_count_idx);
}

