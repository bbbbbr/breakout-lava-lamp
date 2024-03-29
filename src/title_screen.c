#include <gbdk/platform.h>
// #include <gbdk/emu_debug.h>
#include <stdint.h>
#include <stdbool.h>

// #include <audio.h>

// Graphics
#include "res/title_screen_bg.h"
#include "res/title_screen_sprites.h"

#include "input.h"
#include "common.h"

#include "title_screen.h"


#define SPR_OFST_X(val) ((uint8_t)val + (uint8_t)DEVICE_SPRITE_PX_OFFSET_X)


uint8_t current_cursor;

const uint8_t cursors_action_y = 54u + (uint8_t)DEVICE_SPRITE_PX_OFFSET_Y;
const uint8_t cursors_action_x[3] = {SPR_OFST_X(33u),
                                     SPR_OFST_X(33u + (43u * 1u)),
                                     SPR_OFST_X(33u + (43u * 2u)) };

const uint8_t cursors_speed_y = 78u + (uint8_t)DEVICE_SPRITE_PX_OFFSET_Y;
const uint8_t cursors_speed_x[4] = {SPR_OFST_X(29u),
                                    SPR_OFST_X(29u + (32u * 1u)),
                                    SPR_OFST_X(29u + (32u * 2u)),
                                    SPR_OFST_X(29u + (32u * 3u)) };

const uint8_t cursors_players_y = 101u + (uint8_t)DEVICE_SPRITE_PX_OFFSET_Y;
const uint8_t cursors_players_x[5] = {SPR_OFST_X(29u),
                                      SPR_OFST_X(29u + (24u * 1u)),
                                      SPR_OFST_X(29u + (24u * 2u)),
                                      SPR_OFST_X(29u + (24u * 3u)),
                                      SPR_OFST_X(29u + (24u * 4u)) };

const uint8_t action_values[3] = {ACTION_CONTINUE_VAL, ACTION_STD_RANDOM_VAL, ACTION_RANDOM_VAL};
const uint8_t speed_values[4]  = {SPEED_SLOWEST_VAL, SPEED_SLOW_VAL, SPEED_FAST_VAL, SPEED_FASTEST_VAL};
const uint8_t player_values[5] = {PLAYERS_2_VAL, PLAYERS_4_VAL, PLAYERS_8_VAL, PLAYERS_16_VAL, PLAYERS_32_VAL};

const uint8_t cursor_max[3] = {ARRAY_LEN(cursors_action_x)  - 1u,
                               ARRAY_LEN(cursors_speed_x)   - 1u,
                               ARRAY_LEN(cursors_players_x) - 1u};

// These should get loaded from save data
uint8_t cursors_selected[3];


uint8_t settings_get_setting_index(const uint8_t setting_type) {

    if (setting_type <= MENU_MAX) {
        for (uint8_t c = 0; c <= cursor_max[setting_type]; c++) {

            switch (setting_type) {
                case MENU_ACTION:
                    if (action_values[c] == gameinfo.action) {
                        return c; // Success, return index
                    }
                    break;

                case MENU_SPEED:
                    if (speed_values[c] == gameinfo.speed) {
                        return c; // Success, return index
                    }
                    break;

                case MENU_PLAYERS:
                    if (player_values[c] == gameinfo.player_count) {
                        return c; // Success, return index
                    }
                    break;
            }
        }
    }
    return SETTING_VALUE_NOT_FOUND;
}


static void settings_load(void) {
    uint8_t result;

    // Set default cursor
    current_cursor = MENU_DEFAULT;

    result = settings_get_setting_index(MENU_ACTION);
    if (result != SETTING_VALUE_NOT_FOUND) cursors_selected[MENU_ACTION] = result;

    result = settings_get_setting_index(MENU_SPEED);
    if (result != SETTING_VALUE_NOT_FOUND) cursors_selected[MENU_SPEED] = result;

    result = settings_get_setting_index(MENU_PLAYERS);
    if (result != SETTING_VALUE_NOT_FOUND) cursors_selected[MENU_PLAYERS] = result;
}


static void settings_apply(void) {
    gameinfo.action       = action_values[ cursors_selected[MENU_ACTION]  ];
    gameinfo.speed        = speed_values[   cursors_selected[MENU_SPEED]   ];
    gameinfo.player_count = player_values[ cursors_selected[MENU_PLAYERS] ];

    // This is turned off now that in-game player count changing is allowed
    /*
    // Check if game board needs a reset due to player count change
    if (gameinfo.player_count != gameinfo.player_count_last) {
        gameinfo.is_initialized = false;
        gameinfo.player_count_last = gameinfo.player_count;
    }
    */
}


#define SELECT_CURSOR_SPRITE(check_val) ((current_cursor == check_val)  ? SPR_CURSOR_ACTIVE : SPR_CURSOR_NOT_ACTIVE)

static void cursors_redraw(void) {

    set_sprite_tile(SPR_ID_ACTION, SELECT_CURSOR_SPRITE(MENU_ACTION));
    move_sprite(SPR_ID_ACTION, cursors_action_x[ cursors_selected[MENU_ACTION] ], cursors_action_y);

    set_sprite_tile(SPR_ID_SPEED, SELECT_CURSOR_SPRITE(MENU_SPEED));
    move_sprite(SPR_ID_SPEED, cursors_speed_x[ cursors_selected[MENU_SPEED] ], cursors_speed_y);

    set_sprite_tile(SPR_ID_PLAYERS, SELECT_CURSOR_SPRITE(MENU_PLAYERS));
    move_sprite(SPR_ID_PLAYERS, cursors_players_x[ cursors_selected[MENU_PLAYERS] ], cursors_players_y);
}


static void cursors_update(uint8_t keys_ticked) {

    switch (keys_ticked & (J_DPAD)) {

        case J_LEFT:
            if (cursors_selected[current_cursor] > 0)
                cursors_selected[current_cursor]--;
            break;

        case J_RIGHT:
            if (cursors_selected[current_cursor] < cursor_max[current_cursor])
                cursors_selected[current_cursor]++;
            break;

        case J_UP:
            if (current_cursor > 0)
                current_cursor--;
            break;

        case J_DOWN:
            if (current_cursor < MENU_MAX)
                current_cursor++;
            break;
    }
    cursors_redraw();
}


void title_init(void) {

    // Load menu config from settings data
    settings_load();

    // Background image tiles and map
    set_bkg_data(0, title_screen_bg_TILE_COUNT, title_screen_bg_tiles);
    set_bkg_tiles(0,0, title_screen_bg_WIDTH / title_screen_bg_TILE_W,
                       title_screen_bg_HEIGHT / title_screen_bg_TILE_H,
                       title_screen_bg_map);

    // Cursor Sprite tiles
    set_sprite_data(0,title_screen_sprites_TILE_COUNT, title_screen_sprites_tiles);

    hide_sprites_range(0, MAX_HARDWARE_SPRITES - 1);
    SHOW_SPRITES;

    // Set up cursors
    cursors_redraw();
}


#define IDLE_AUTO_START_THRESHOLD (1u * (60u *60u)) // 1 minutes * 60 seconds per minute * ~60 frames per second
#define IDLE_RESET 0u;

void title_run(void) {

    uint16_t idle_count = 0;

    UPDATE_KEYS();

    // First half of random init
    // (in case they don't click around after returning to the menu
    //  when the menu item is already on "Random" and so misses that seed)
    // This will always be the same on startup,
    // but thereafter differs when the user returns to the menu
    gameinfo.user_rand_seed.l = DIV_REG;

    // Idle until user presses any button
    while (1) {

        UPDATE_KEYS();
        idle_count++;

        if (KEY_TICKED(J_DPAD)) {

            // More first half of random init (updated with user moving around the menu)
            // Reset idle counter if users presses any menu buttons
            gameinfo.user_rand_seed.l = DIV_REG;
            idle_count = IDLE_RESET;
            cursors_update( GET_KEYS_TICKED(J_DPAD) );
        }
        else if ((KEY_TICKED(J_START | J_A | J_B)) ||
                 (idle_count > IDLE_AUTO_START_THRESHOLD)) {

            // Exit menu and start animation if user presses a given button
            // or the idle counter crosses the threshold
            // Second half of random init (updated with user exiting the menu)
            gameinfo.user_rand_seed.h = DIV_REG;

            settings_apply();
            return;
        }

        vsync();
    }
}


