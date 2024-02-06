#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>
#include <stdbool.h>

// #define DEBUG_ENABLED

#include "gameboard.h"

#define ARRAY_LEN(A)  sizeof(A) / sizeof(A[0])

// #define STATE_SHOWSPLASH  1u
#define STATE_SHOWTITLE   0u
#define STATE_RUNGAME     1u

#define RAND_SEED_STANDARD 0x1234u

// == See "save_and_restore.c" for where defaults are loaded ==

#define PLAYER_TEAMS_COUNT   4u    // 4 teams with different colors
#define PLAYER_TEAMS_MASK 0x03u // Bitmask for reducing counters down to a member of a given team

#define ACTION_CONTINUE    0u
#define ACTION_STD_RANDOM  1u
#define ACTION_RANDOM      2u
    #define ACTION_CONTINUE_VAL    0u
    #define ACTION_STD_RANDOM_VAL  1u
    #define ACTION_RANDOM_VAL      2u

#define SPEED_SLOWEST  0u
#define SPEED_SLOW     1u
#define SPEED_FAST     2u
#define SPEED_FASTEST  3u
    // Player position is in Fixed 8.8, speed is signed 8 bit
    // Should be <= than 16
    #define SPEED_SLOWEST_VAL  1u
    #define SPEED_SLOW_VAL     2u
    #define SPEED_FAST_VAL     4u
    #define SPEED_FASTEST_VAL  8u

    #define SPEED_MIN (SPEED_SLOWEST_VAL)
    #define SPEED_MAX (SPEED_FASTEST_VAL)


#define PLAYERS_2      0u
#define PLAYERS_4      1u
#define PLAYERS_8      2u
#define PLAYERS_16     3u
#define PLAYERS_32     4u
    #define PLAYERS_2_VAL      2u
    #define PLAYERS_4_VAL      4u
    #define PLAYERS_8_VAL      8u
    #define PLAYERS_16_VAL    16u
    #define PLAYERS_32_VAL    32u

    #define PLAYER_COUNT_MIN    (PLAYERS_2_VAL)
    #define PLAYER_COUNT_MAX    (PLAYERS_32_VAL)

#define BOARD_W (DEVICE_SCREEN_WIDTH)   // In Tiles
#define BOARD_H (DEVICE_SCREEN_HEIGHT)  // In Tiles


#define BOARD_GRID_SZ 8u

// Min/Max are screen borders less sprite width
#define SPRITE_WIDTH  8u
#define SPRITE_HEIGHT 8u

#define PLAYER_MIN_X_U16 ((                    0u + (SPRITE_WIDTH / 2)) << 8)
#define PLAYER_MAX_X_U16 ((DEVICE_SCREEN_PX_WIDTH - (SPRITE_WIDTH / 2)) << 8)

#define PLAYER_MIN_Y_U16 ((                     0u + (SPRITE_HEIGHT / 2)) << 8)
#define PLAYER_MAX_Y_U16 ((DEVICE_SCREEN_PX_HEIGHT - (SPRITE_HEIGHT / 2)) << 8)

#define PLAYER_RANGE_X_U8 (DEVICE_SCREEN_PX_WIDTH - SPRITE_WIDTH)
#define PLAYER_RANGE_Y_U8 (DEVICE_SCREEN_PX_HEIGHT - SPRITE_HEIGHT)

#define BOARD_COL_WHITE  0u
#define BOARD_COL_BLACK  3u
#define BOARD_COL_D_GREY 2u
#define BOARD_COL_L_GREY 1u

#define PLAYER_SPR_COL_BLACK 0u
#define PLAYER_SPR_COL_WHITE 1u


// === GAME STATE AND SAVE DATA ===


// Save record signature check
#define SAVEDATA_SIG_CHECK_0 0xA50Fu
#define SAVEDATA_SIG_CHECK_1 0x1E78u

#define SAVE_VERSION_NUM   2u // Increment when making breaking changes to save data structure


typedef struct player_t {
    fixed x;
    fixed y;
    fixed next_x;
    fixed next_y;
    int16_t speed_x;
    int16_t speed_y;
    uint8_t angle;
    bool bounce_x;
    bool bounce_y;
} player_t;


// See save_and_restore.c for default initialization
//
typedef struct gameinfo_t {
    // Don't change order, it will scramble cart saves
    uint8_t  save_checksum;
    uint16_t save_check0;
    uint16_t save_check1;
    uint8_t save_version;

    // Options
    uint8_t player_count;
    uint8_t player_count_last;
    uint8_t speed;
    uint8_t action;

    bool sprites_enabled;
    bool enable_sprites_next_frame;

    // State
    bool    is_initialized;
    fixed   user_rand_seed;
    uint16_t system_rand_seed_saved;

    player_t players[PLAYER_COUNT_MAX];
    uint8_t board[BOARD_W * BOARD_H];

    // Need to store and re-load the random seed?

} gameinfo_t;

extern gameinfo_t gameinfo;

extern const uint8_t board_team_bg_colors[PLAYER_TEAMS_COUNT];
extern const uint8_t player_sprite_colors[PLAYER_TEAMS_COUNT];


#endif // _COMMON_H


