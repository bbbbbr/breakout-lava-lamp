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
    for (uint8_t y = 0; y < BOARD_H; y++) {
        for (uint8_t x = 0; x < BOARD_W; x++) {

            if (gameinfo.player_count == PLAYERS_4_VAL) {

                // Divide board into 4 color regions, Left & Right and Top & Bottom
                *p_board = board_player_colors[ (x / (BOARD_W / 2)) + ((y / (BOARD_H / 2)) * 2u) ];
            }
            else { // Default, implied: PLAYERS_2_VAL

                // Divide board into 2 color regions, Left & Right
                *p_board = board_player_colors[ x / (BOARD_W / 2) ]; // x + (y * BOARD_W);
            }
            p_board++;
        }
    }
}


// Expects screen faded out
static void board_init_gfx(void) {
    // Palette loading is handled by the fade routine

    // Load the tiles
    set_bkg_data(0,    bg_tile_patterns_length  / TILE_PATTERN_SZ, bg_tile_patterns);

    set_sprite_data(0, spr_tile_patterns_length / TILE_PATTERN_SZ, spr_tile_patterns);

    // Draw the board
    set_bkg_tiles(0u, 0u, BOARD_W, BOARD_H, gameinfo.board);

    hide_sprites_range(0u, MAX_HARDWARE_SPRITES - 1);

    for (uint8_t c = 0; c < gameinfo.player_count; c++) {
        set_sprite_tile(c, player_colors[c & 0x03u]);
    }

    players_redraw_sprites();
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


// Load graphics and do an initial redraw + recalc player X/Y speeds
// Expects Board and Players to have been initialized
void board_init(void) {

    board_init_gfx();

    // Always recalc player speeds when start up
    // in case speed was changed in the menu
    players_all_recalc_movement();
}


void board_run(void) {

    uint8_t save_counter = 0u;

    // Always reset action to "Continue" once a board has been started
    // so that when it gets restored after power-on it defaults to continuing
    gameinfo.action = ACTION_CONTINUE_VAL;

    while(TRUE) {
        UPDATE_KEYS();

        players_update();

        // Skip Vsync while SELECT is pressed
        if (!(KEY_PRESSED(J_SELECT)))
            vsync();

        // Return to Title menu if some keys pressed
        // + save game state
        if (KEY_TICKED(J_START | J_A | J_B)) {
            savedata_save();
            return;
        }

        // Save game state periodically
        save_counter++;
        if (save_counter >= SAVE_FRAMES_THRESHOLD) {
            save_counter = 0u;
            savedata_save();
        }
    }
}