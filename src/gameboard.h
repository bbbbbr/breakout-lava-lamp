#ifndef __GAMEBOARD_H
#define __GAMEBOARD_H

#define BOARD_W (DEVICE_SCREEN_WIDTH)   // In Tiles
#define BOARD_H (DEVICE_SCREEN_HEIGHT)  // In Tiles

 // Player position is in Fixed 8.8, speed is signed 8 bit
// Should be <= than 16
#define PLAYER_SPEED_DEFAULT 4
#define PLAYER_SPEED_MIN     1
#define PLAYER_SPEED_MAX     8

#define BOARD_GRID_SZ 8u

// Min/Max are screen borders less sprite width
#define SPRITE_WIDTH  8u
#define SPRITE_HEIGHT 8u

#define PLAYER_MIN_X_U16 ((                    0u + (SPRITE_WIDTH / 2)) << 8)
#define PLAYER_MAX_X_U16 ((DEVICE_SCREEN_PX_WIDTH - (SPRITE_WIDTH / 2)) << 8)

#define PLAYER_MIN_Y_U16 ((                     0u + (SPRITE_HEIGHT / 2)) << 8)
#define PLAYER_MAX_Y_U16 ((DEVICE_SCREEN_PX_HEIGHT - (SPRITE_HEIGHT / 2)) << 8)


#define PLAYER_COUNT_2 2u
#define PLAYER_COUNT_4 4u
#define PLAYER_COUNT_DEFAULT (PLAYER_COUNT_2)
#define PLAYER_COUNT_MAX     (PLAYER_COUNT_4)

#define BOARD_COL_WHITE  0u
#define BOARD_COL_BLACK  3u
#define BOARD_COL_D_GREY 2u
#define BOARD_COL_L_GREY 1u

#define PLAYER_COL_BLACK 0u
#define PLAYER_COL_WHITE 1u

void board_init(void);
void board_run(void);

#endif // __GAMEBOARD_H


