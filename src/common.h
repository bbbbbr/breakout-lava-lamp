#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>
#include <stdbool.h>

// #define DEBUG_ENABLED

#include "gameboard.h"

#define ARRAY_LEN(A)  sizeof(A) / sizeof(A[0])

#define STATE_STARTUP     0u
#define STATE_SHOWSPLASH  1u
#define STATE_SHOWTITLE   2u
#define STATE_RUNGAME     3u
#define STATE_GAMEDONE    4u

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
    uint8_t state;
    uint8_t player_count;

    player_t players[PLAYER_COUNT_MAX];
    uint8_t board[BOARD_W * BOARD_H];

} gameinfo_t;

extern gameinfo_t gameinfo;

#endif // _COMMON_H


