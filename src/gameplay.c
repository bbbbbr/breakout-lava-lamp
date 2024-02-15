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
#include "players_asm.h"



// Toggle sprites on/off
// (both display of any sprites and updating their oam positions)
inline void toggle_sprites_enabled(void) {

    gameinfo.sprites_enabled = !gameinfo.sprites_enabled;

    if (gameinfo.sprites_enabled == false)
        HIDE_SPRITES;
    else
        gameinfo.enable_sprites_next_frame = true;
}


// Alter some options while the board is running using the D-Pad
static void handle_dpad_options(uint8_t keys_ticked) {
    switch (keys_ticked) {
        // Option adjustments assume always powers of 2
        // to easily shift up/down between valid settings

        case J_RIGHT:
            // Speed Up
            if (gameinfo.speed < SPEED_MAX) gameinfo.speed <<= 1;
            break;

        case J_LEFT:
            // Speed Down
            if (gameinfo.speed > SPEED_MIN) gameinfo.speed >>= 1;
            break;

        // Number of Players Up/Down
        case J_DOWN:
            // Less Players
            if (gameinfo.player_count > PLAYER_COUNT_MIN) gameinfo.player_count >>= 1;
            // Hide any sprites that might no longer be needed
             hide_sprites_range(gameinfo.player_count, MAX_HARDWARE_SPRITES - 1);
            break;

        case J_UP:
            // More Players
            if (gameinfo.player_count < PLAYER_COUNT_MAX) gameinfo.player_count <<= 1;
            break;
    }

    // Select current speed*sine table
    sine_table_select(gameinfo.speed);

    players_all_recalc_movement();
}


// Load graphics and do an initial redraw + recalc player X/Y speeds
// Expects Board and Players to have been initialized
void game_init(void) {

    // Select current speed*sine table
    sine_table_select(gameinfo.speed);

    // Reset the board if needed or if the user is NOT Continuing
    if ((gameinfo.is_initialized == false) || (gameinfo.action != ACTION_CONTINUE_VAL)) {
        gameinfo.is_initialized = true;
        gameinfo.sprites_enabled = true;
        board_reset();
    }

    board_init_gfx();

    // Always recalc player speeds during start up
    // in case the user changed the speed option in the menu
    players_all_recalc_movement();
}


#define SPEEDUP_SELECT_HELD_THRESHOLD 20u  // ~1/3 of a second

void game_run(void) {

    #ifdef DEBUG_FIXED_RUN_TIME
        uint8_t frames_to_run = DEBUG_FIXED_RUN_TIME;
    #endif

    #ifdef FEATURE_AUTO_SAVE
        uint8_t save_counter = 0u;
    #endif

    uint8_t keys_ticked;
    uint8_t select_held_count = 0u;
    bool    paused = false;

    // Always reset action to "Continue" once a board has been started
    // so that when it gets restored after power-on it defaults to continuing
    gameinfo.action = ACTION_CONTINUE_VAL;

    // Make a save right before starting
    savedata_save();

    while(TRUE) {

        #ifdef DEBUG_FIXED_RUN_TIME
            if (frames_to_run == 0)
                while(1);
            frames_to_run--;
        #endif


        UPDATE_KEYS();
        keys_ticked = GET_KEYS_TICKED(J_ANY);

        // Return to Title menu if some keys pressed + save game state
        if (keys_ticked & (J_A | J_B)) {
            savedata_save();
            return;
        }
        else if (keys_ticked & (J_DPAD)) {
            // Adjust some game settings live with the D-Pad
            handle_dpad_options(keys_ticked);
        }
        else if (keys_ticked & (J_START)) {
            // Toggle pause, make a save if entering paused state
            // TODO: Consider making it part of saved global game state/settings
            paused = !paused;
            if (paused)
                savedata_save();
        }

        // Sprite Toggle and Speed Up
        if (KEY_PRESSED(J_SELECT)) {
            // Skip Vsync if SELECT is pressed for long enough (Temporary Speed Up)
            if (select_held_count < SPEEDUP_SELECT_HELD_THRESHOLD) {
                select_held_count++;
                vsync();
            }
        }
        else {
           vsync();

            // If SELECT was just short-pressed use it to toggle sprites on/off
            if (KEY_RELEASED(J_SELECT)) {
                if (select_held_count < SPEEDUP_SELECT_HELD_THRESHOLD) {
                    toggle_sprites_enabled();
                }
            }
                 // Reset counter when button is released
            select_held_count = 0u;
        }

        players_apply_queued_vram_updates();

        if (!paused) {
            // players_update();
            players_update_v5();
            // players_update_asm();

            // Redraw the sprites if needed
            if (gameinfo.sprites_enabled) players_redraw_sprites_asm();
            // if (gameinfo.sprites_enabled) players_redraw_sprites();
        }

        // Turn sprites back on after they've had a chance to update in players_update()
        // so they don't visibly pop from the old location to new
        if (gameinfo.enable_sprites_next_frame) {
            vsync();
            SHOW_SPRITES;
            gameinfo.enable_sprites_next_frame = false;
        }

        #ifdef FEATURE_AUTO_SAVE
            // Save game state periodically
            // Disabled since this can cause a big cpu spike per frame due to large memcopy.
            // Instead rely on the sace during pause / return to menu and remove this
            save_counter++;
            if (save_counter >= SAVE_FRAMES_THRESHOLD) {
                save_counter = 0u;
                savedata_save();
            }
        #endif
    }
}