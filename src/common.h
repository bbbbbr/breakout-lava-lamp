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

#define PLAYERS_2      0u
#define PLAYERS_4      1u
    #define PLAYERS_2_VAL      2u
    #define PLAYERS_4_VAL      4u
    #define PLAYER_COUNT_MIN    (PLAYERS_2_VAL)
    #define PLAYER_COUNT_MAX    (PLAYERS_4_VAL)

// Save record signature check
#define SAVEDATA_SIG_CHECK_0 0xA50Fu
#define SAVEDATA_SIG_CHECK_1 0x1E78u

#define SAVE_VERSION_NUM   1u // Increment when making breaking changes to save data structure


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


typedef struct gameinfo_t {
    // Don't change order, it will scramble cart saves
    uint8_t  save_checksum;
    uint16_t save_check0;
    uint16_t save_check1;
    uint8_t save_version;

    uint8_t player_count;
    uint8_t player_count_last;
    uint8_t speed;
    uint8_t action;
    bool    is_initialized;
    fixed   user_rand_seed;
    uint16_t system_rand_seed_saved;

    player_t players[PLAYER_COUNT_MAX];
    uint8_t board[BOARD_W * BOARD_H];

    // Need to store and re-load the random seed?

} gameinfo_t;

extern gameinfo_t gameinfo;

#endif // _COMMON_H


