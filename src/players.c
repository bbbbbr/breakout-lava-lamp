#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
#include <gbdk/emu_debug.h>
#include <rand.h>

#include "common.h"
#include "input.h"
#include "gfx.h"
#include "math_util.h"
#include "save_and_restore.h"

#include "gameboard.h"
#include "players_asm.h"

// #define EMU_PRINT_ON

// offset to center the 8X8 circle sprite on the x,y coordinate
#define SPR_OFFSET_X (DEVICE_SPRITE_PX_OFFSET_X)
#define SPR_OFFSET_Y (DEVICE_SPRITE_PX_OFFSET_Y)



// Player corner calculations (Sprite is 8 pixels wide, Upper Left is 0,0, Lower Right is +7,+7)
#define PLAYER_LEFT(px)   ((px                     ) / BOARD_GRID_SZ)
#define PLAYER_RIGHT(px)  ((px + (SPRITE_WIDTH - 1)) / BOARD_GRID_SZ)
#define PLAYER_TOP(py)    ((py                     ) / BOARD_GRID_SZ)
#define PLAYER_BOTTOM(py) ((py + (SPRITE_HEIGHT - 1)) / BOARD_GRID_SZ)


uint16_t board_update_queue[PLAYER_TEAMS_COUNT];
uint8_t board_update_count;

// ====== PLAYERS ======

void players_redraw_sprites(void) {

    player_t * p_player = &gameinfo.players[0];

    for (uint8_t c = 0; c < gameinfo.player_count; c++) {
        move_sprite(c, p_player->x.h + SPR_OFFSET_X,
                       p_player->y.h + SPR_OFFSET_Y);
        p_player++;
    }
}


// Recalc X,Y speed based on Angle and Speed for a given player
void player_recalc_movement(uint8_t idx) {
    uint8_t t_angle = gameinfo.players[idx].angle;

    gameinfo.players[idx].speed_x = (int16_t)SIN(t_angle);
    // Flip Y direction since adding positive values moves further down the screen
    gameinfo.players[idx].speed_y = (int16_t)COS(t_angle) * -1;

    // Old version that didn't pre-calculate Angle * Speed
    //
    // gameinfo.players[idx].speed_x = (int16_t)SIN(t_angle) * gameinfo.speed;
    // // Flip Y direction since adding positive values moves further down the screen
    // gameinfo.players[idx].speed_y = (int16_t)COS(t_angle) * gameinfo.speed * -1;
}


// Recalc X,Y speed based on Angle and Speed for all players
void players_all_recalc_movement(void) {
        // Calculate default speed based on angles and uniform speed
    for (uint8_t c = 0; c < gameinfo.player_count; c++) {
        player_recalc_movement(c);
    }
}


// Check to see if a given board tile does not match the player (collision) or matches (no collision)
// TODO: OPTIMIZE: could inline to improve performance
bool player_board_check_xy(uint8_t x, uint8_t y, uint8_t player_team_col) {
    bool collision = false;

    // This is a wall collision, so no tile updates
    // Unsigned wraparound to negative is also handled by this
    if ((x >= BOARD_DISP_W) || (y >= BOARD_DISP_H)) {
         // Return false here since it's off the grid
         // It's ok to do that since the player sprite is not bigger than a tile
         // so if one edge is off a remaining corner will still get checked at the right location
        return false;
    }

    uint16_t board_index = x + (y * BOARD_BUF_W);
    if (gameinfo.board[board_index] != player_team_col) {
        // Don't update the board itself here since it needs to remain unmodified
        // until both Horizontal and Vertical testing is done, so queue an update
        board_update_queue[board_update_count++] = board_index;
        // OPTIONAL: queue a tile draw instead of handling immediately
        set_vram_byte(_SCRN0 + board_index, player_team_col);
        collision = true;
    }

    return collision;
}

/////////////////////////////////////////////////////////////////////////////
// V5

uint16_t board_index_v5;  // Faster with a global than stack local
uint8_t players_count_v5;

void players_update_v5(void) {

    players_count_v5 = gameinfo.player_count;
    player_t * p_player = &gameinfo.players[0];

    for (uint8_t c = 0; c < players_count_v5; c++) {

        uint8_t player_team_color = c & PLAYER_TEAMS_MASK;
        board_update_count = 0;

        // Calculate next position (bounce flags get reset in test code)
        p_player->next_x.w = p_player->x.w + p_player->speed_x;
        p_player->next_y.w = p_player->y.w + p_player->speed_y;


        // Check Wall Collisions (out of bounds)

            // Check Horizontal movement
            if (p_player->next_x.h > PLAYER_MAX_X_U8) {
                p_player->bounce_x = true;
            } else
                p_player->bounce_x = false;

            // Check Vertical movement
            if (p_player->next_y.h > PLAYER_MAX_Y_U8) {
                p_player->bounce_y = true;
            } else
                p_player->bounce_y = false;


        // Check board collisions

            // Check Horizontal movement (New X & Current Y in order to avoid false triggers on Y)
            // Test separately here since collision check also updates board tile color
            if ((p_player->speed_x != 0) && (p_player->bounce_x == false)) {

                // Faster with the stack local vars instead of a compounded statement with index calc
                uint8_t test_next_x = (p_player->speed_x > 0) ? PLAYER_RIGHT(p_player->next_x.h) : PLAYER_LEFT(p_player->next_x.h);
                // TODO: if ((p_player->speed_x > 0) && (p_player->y.h & 0x07u)) // Move to right tile if needed
                uint8_t test_y = PLAYER_TOP(p_player->y.h);

                board_index_v5 = (test_next_x) + ((test_y) * BOARD_BUF_W);

               if (gameinfo.board[board_index_v5] != player_team_color) {
                    // Don't update the board itself here since it needs to remain unmodified
                    // until both Horizontal and Vertical testing is done, so queue an update
                    board_update_queue[board_update_count++] = board_index_v5;
                    // OPTIONAL: queue a tile draw instead of handling immediately
                    set_vram_byte(_SCRN0 + board_index_v5, player_team_color);
                    p_player->bounce_x = true;
                }

                // Only check Bottom Right if ball spans more than 2 tiles
                // (i.e is not evenly aligned on grid boundary with x % 8 == 0)
                if (p_player->y.h & 0x07u) {
                    // Next Tile Row on board
                    board_index_v5 += BOARD_BUF_W;

                   if (gameinfo.board[board_index_v5] != player_team_color) {
                        board_update_queue[board_update_count++] = board_index_v5;
                        set_vram_byte(_SCRN0 + board_index_v5, player_team_color);
                        p_player->bounce_x = true;
                    }
                }
            }

            // Check Vertical movement (New Y & Current X in order to avoid false triggers on X)
            if ((p_player->speed_y != 0) && (p_player->bounce_y == false)) {

                uint8_t test_x = PLAYER_LEFT(p_player->x.h);
                uint8_t test_next_y = (p_player->speed_y > 0) ? PLAYER_BOTTOM(p_player->next_y.h) : PLAYER_TOP(p_player->next_y.h);

                board_index_v5 = ( test_x) + (( test_next_y) * BOARD_BUF_W);

               if (gameinfo.board[board_index_v5] != player_team_color) {
                    // Don't update the board itself here since it needs to remain unmodified
                    // until both Horizontal and Vertical testing is done, so queue an update
                    board_update_queue[board_update_count++] = board_index_v5;
                    // OPTIONAL: queue a tile draw instead of handling immediately
                    set_vram_byte(_SCRN0 + board_index_v5, player_team_color);
                    p_player->bounce_y = true;
                }

                // Only check Right side if ball spans more than 2 tiles
                // (i.e is not evenly aligned on grid boundary with x % 8 == 0)
                if (p_player->x.h & 0x07u) {
                    // Next Tile Column on board
                    board_index_v5 ++;

                   if (gameinfo.board[board_index_v5] != player_team_color) {
                        board_update_queue[board_update_count++] = board_index_v5;
                        set_vram_byte(_SCRN0 + board_index_v5, player_team_color);
                        p_player->bounce_y = true;
                    }
                }
            }


            // Apply queued board updates
            while (board_update_count) {
                gameinfo.board[ board_update_queue[--board_update_count] ] = player_team_color;
            }

//////////////////////////

        // If there was a collision then calculate bounce angle
        // and don't update position
        if (p_player->bounce_x || p_player->bounce_y) {

            if (p_player->bounce_x)
                p_player->angle = (uint8_t)(ANGLE_TO_8BIT(360) - p_player->angle);

            if (p_player->bounce_y)
                p_player->angle = (uint8_t)(ANGLE_TO_8BIT(180) - p_player->angle);

            // Slightly perturb the player angle if there was a collision
            p_player->angle = (uint8_t)(p_player->angle + (int8_t)(rand() & 0x03u) - 1);
            // player_recalc_movement(c);

                p_player->speed_x = (int16_t)SIN(p_player->angle);
                // Flip Y direction since adding positive values moves further down the screen
                p_player->speed_y = (int16_t)COS(p_player->angle) * -1;

        } else {
            // Otherwise update position to new location
            // Apply the new delta movement
            p_player->x.w = p_player->next_x.w;
            p_player->y.w = p_player->next_y.w;
        }
        p_player++;
    }
}





/////////////////////////////////////////////////////////////////////////////

// V4
/*
uint16_t board_index_v4;

void player_check_board_collisions_v4(uint8_t player_id, player_t * p_player) {


    uint8_t player_team_col = player_id & PLAYER_TEAMS_MASK;

    // Check Wall Collisions (out of bounds)

        // Check Horizontal movement
        if (p_player->next_x.h > PLAYER_MAX_X_U8) {
            p_player->bounce_x = true;
            return;
        } else
            p_player->bounce_x = false;

        // Check Vertical movement
        if (p_player->next_y.h > PLAYER_MAX_Y_U8) {
            p_player->bounce_y = true;
            return;
        } else
            p_player->bounce_y = false;


    // Check Board collisions


// WARNING            THIS IS NOT TRUE WHEN MOVING MORE THAN 1 PIXEL AT A TIME




    // uint8_t cur_x = p_player->x.h / BOARD_GRID_SZ;
    // uint8_t cur_y = p_player->y.h / BOARD_GRID_SZ;
    // uint8_t next_x = p_player->next_x.h / BOARD_GRID_SZ;
    // uint8_t next_y = p_player->next_y.h / BOARD_GRID_SZ;

// OOOOOHHHHHH
//  In a crowded board, if you don't check every frame all around, then
//  other balls re-setting tiles UNDERNEATH another ball could cause
// a problem for that ball -- since the optimized version won't check
// again until some certain thresholds are crossed
    // More of a thing on "contested edges" where two balls from opposite sides strike often
// ALWAYS CHECK EVERY FRAME AND EVERY CORNER??
            // use tests below to determine AXIS OF COLLISION ONLY?
                // maybe possible to still only do tests at new location?
                        // THere is something wrong with movement tests at EDGES of screen?


// TODO: check out of bounds and emu debug report it
    // NOTE: This only detects movement on scale of 1 pixel at a time
    //
    // Determine whether each axis had movement to test for collisions:
    //
    // * The previous position was perfectly grid 8x8 aligned (N & 0x7 == 0)
    //   meaning touching only one tile.
    // * AND the next position is NOT grid aligned (N & 0x7 != 0) which means
    //   on this frame it is entering new tile(s) which need to be checked (only
    //   during the transition frame)
    bool moved_x = ( ((p_player->x.h & 0x07u) == 0u) && ((p_player->next_x.h & 0x07u) != 0u) );
    bool moved_y = ( ((p_player->y.h & 0x07u) == 0u) && ((p_player->next_y.h & 0x07u) != 0u) );

    // ignore if next_x & 0x07u == 0 // (it's evenly aligned, don't care X)

    // or if board scale cur_x != next_x AND the new location spans 2 tiles
    if ( ((p_player->x.h / BOARD_GRID_SZ) != (p_player->next_x.h / BOARD_GRID_SZ))
        // &&
        // ((p_player->next_x.h & 0x07u) != 0u) ) {
        ) {
        moved_x = true;
    }
    if ( ((p_player->y.h / BOARD_GRID_SZ) != (p_player->next_y.h / BOARD_GRID_SZ))
       // &&
       // ((p_player->next_y.h & 0x07u) != 0u) )
        ) {
        moved_y = true;
    }

    // Check for horizontal movement
// if ( moved_x || moved_y) {

        bool collide = false;

        // Check horizontal width
        if (p_player->next_x.h & 0x07u) {

            // Start in Upper-Left corner of board
            board_index_v4 = (p_player->next_x.h / BOARD_GRID_SZ) + ((p_player->next_y.h / BOARD_GRID_SZ) * BOARD_BUF_W);

            // X axis moved, spans 2 board tiles

            // Check horizontal width
            if (p_player->next_y.h & 0x07u) {

                // Y axis moved, spans 2 board tiles
                // Ball spans 2x2 tiles (movement in both directions)

    // TODO: benchmark pointer access to board

                if (gameinfo.board[board_index_v4] != player_team_col) {
                    gameinfo.board[board_index_v4] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index_v4, player_team_col);
                    collide = true;
                }
                // Next tile column
                board_index_v4++;`
                if (gameinfo.board[board_index_v4] != player_team_col) {
                    gameinfo.board[board_index_v4] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index_v4, player_team_col);
                    collide = true;
                }

                // Next Tile Row on board (step back to start X)
                board_index_v4 += (BOARD_BUF_W - 1u);
                if (gameinfo.board[board_index_v4] != player_team_col) {
                    gameinfo.board[board_index_v4] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index_v4, player_team_col);
                    collide = true;
                }
                // Next Tile column
                board_index_v4++;`
                if (gameinfo.board[board_index_v4] != player_team_col) {
                    gameinfo.board[board_index_v4] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index_v4, player_team_col);
                    collide = true;
                }
            }
            else {

                // Y axis DID NOT move
                // Ball spans 2x1 tiles (movement in horizontal direction)

                // Only Left/Right edges touch different tiles
                if (gameinfo.board[board_index_v4] != player_team_col) {
                    gameinfo.board[board_index_v4] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index_v4, player_team_col);
                    collide = true;
                }
                // Next Tile Column
                board_index_v4++;
                if (gameinfo.board[board_index_v4] != player_team_col) {
                    gameinfo.board[board_index_v4] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index_v4, player_team_col);
                    collide = true;
                }
            }
        }
        else {

            // X axis did not move, only spans one board tiles

            // Check horizontal width
            if (p_player->next_y.h & 0x07u) {

                // Start in Upper-Left corner of board
                board_index_v4 = (p_player->next_x.h / BOARD_GRID_SZ) + ((p_player->next_y.h / BOARD_GRID_SZ) * BOARD_BUF_W);

                // Y axis moved, spans 2 board tiles
                // Ball spans 1x2 tiles (movement in vertical direction)

                // Only Top/Bottom edges touch different tiles
                if (gameinfo.board[board_index_v4] != player_team_col) {
                    gameinfo.board[board_index_v4] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index_v4, player_team_col);
                    collide = true;
                }

                // Next Tile Row on board
                board_index_v4 += BOARD_BUF_W;
                if (gameinfo.board[board_index_v4] != player_team_col) {
                    gameinfo.board[board_index_v4] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index_v4, player_team_col);
                    collide = true;
                }
            }
            // else {

            //     // No movement
            //     // EMU_printf("1x1: default")
            // }
        }
        // Apply results
        if (collide) {
            p_player->bounce_x = moved_x;
            p_player->bounce_y = moved_y;
        }
    // }
}





/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// V3

uint16_t board_index_v3;

void player_check_board_collisions_v3(uint8_t player_id, player_t * p_player) {


    uint8_t player_team_col = player_id & PLAYER_TEAMS_MASK;

    // Check Wall Collisions (out of bounds)

        // Check Horizontal movement
        if (p_player->next_x.h > PLAYER_MAX_X_U8) {
            p_player->bounce_x = true;
            return;
        } else
            p_player->bounce_x = false;

        // Check Vertical movement
        if (p_player->next_y.h > PLAYER_MAX_Y_U8) {
            p_player->bounce_y = true;
            return;
        } else
            p_player->bounce_y = false;


    // Check Board collisions

    // uint8_t cur_x = p_player->x.h / BOARD_GRID_SZ;
    // uint8_t cur_y = p_player->y.h / BOARD_GRID_SZ;
    // uint8_t next_x = p_player->next_x.h / BOARD_GRID_SZ;
    // uint8_t next_y = p_player->next_y.h / BOARD_GRID_SZ;


// TODO: check out of buonds and emu debug report it

    // Check for horizontal movement
    if ( ((p_player->x.h & 0x07u) == 0u) &&
         ((p_player->next_x.h & 0x07u) != 0u)) {

        // Start in Upper-Left corner of board
        board_index_v3 = (p_player->next_x.h / BOARD_GRID_SZ) + ((p_player->next_y.h / BOARD_GRID_SZ) * BOARD_BUF_W);

        // X axis moved, spans 2 board tiles

        // Check for vertical movement
        if ( ((p_player->y.h & 0x07u) == 0u) &&
             ((p_player->next_y.h & 0x07u) != 0u)) {

            // Y axis moved, spans 2 board tiles
            // Ball spans 2x2 tiles (movement in both directions)

// TODO: benchmark pointer access to board

            if (gameinfo.board[board_index_v3] != player_team_col) {
                gameinfo.board[board_index_v3] = player_team_col;
                set_vram_byte(_SCRN0 + board_index_v3, player_team_col);
                p_player->bounce_x = true, p_player->bounce_y = true;
            }
            // Next tile column
            board_index_v3++;`
            if (gameinfo.board[board_index_v3] != player_team_col) {
                gameinfo.board[board_index_v3] = player_team_col;
                set_vram_byte(_SCRN0 + board_index_v3, player_team_col);
                p_player->bounce_x = true, p_player->bounce_y = true;
            }

            // Next Tile Row on board (step back to start X)
            board_index_v3 += (BOARD_BUF_W - 1u);
            if (gameinfo.board[board_index_v3] != player_team_col) {
                gameinfo.board[board_index_v3] = player_team_col;
                set_vram_byte(_SCRN0 + board_index_v3, player_team_col);
                p_player->bounce_x = true, p_player->bounce_y = true;
            }
            // Next Tile column
            board_index_v3++;`
            if (gameinfo.board[board_index_v3] != player_team_col) {
                gameinfo.board[board_index_v3] = player_team_col;
                set_vram_byte(_SCRN0 + board_index_v3, player_team_col);
                p_player->bounce_x = true, p_player->bounce_y = true;
            }
        }
        else {

            // Y axis DID NOT move
            // Ball spans 2x1 tiles (movement in horizontal direction)

            // Only Left/Right edges touch different tiles
            if (gameinfo.board[board_index_v3] != player_team_col) {
                gameinfo.board[board_index_v3] = player_team_col;
                set_vram_byte(_SCRN0 + board_index_v3, player_team_col);
                p_player->bounce_x = true;
            }
            // Next Tile Column
            board_index_v3++;
            if (gameinfo.board[board_index_v3] != player_team_col) {
                gameinfo.board[board_index_v3] = player_team_col;
                set_vram_byte(_SCRN0 + board_index_v3, player_team_col);
                p_player->bounce_x = true;
            }
        }
    }
    else {

        // X axis did not move, only spans one board tiles

        // Check for vertical movement
        if ( ((p_player->y.h & 0x07u) == 0u) &&
             ((p_player->next_y.h & 0x07u) != 0u)) {

            // Start in Upper-Left corner of board
            board_index_v3 = (p_player->next_x.h / BOARD_GRID_SZ) + ((p_player->next_y.h / BOARD_GRID_SZ) * BOARD_BUF_W);

            // Y axis moved, spans 2 board tiles
            // Ball spans 1x2 tiles (movement in vertical direction)

            // Only Top/Bottom edges touch different tiles
            if (gameinfo.board[board_index_v3] != player_team_col) {
                gameinfo.board[board_index_v3] = player_team_col;
                set_vram_byte(_SCRN0 + board_index_v3, player_team_col);
                p_player->bounce_y = true;
            }

            // Next Tile Row on board
            board_index_v3 += BOARD_BUF_W;
            if (gameinfo.board[board_index_v3] != player_team_col) {
                gameinfo.board[board_index_v3] = player_team_col;
                set_vram_byte(_SCRN0 + board_index_v3, player_team_col);
                p_player->bounce_y = true;
            }
        }
        // else {

        //     // No movement
        //     // EMU_printf("1x1: default")
        // }
    }
}


*/

/*
/////////////////////////////////////////////////////////////////////////////

// V2

// #define SPAN_MULTI_TILE_X(px) ((px & 0x07u) != 0x04u)
// #define SPAN_MULTI_TILE_Y(py) ((py & 0x07u) != 0x04u)
#define SPAN_MULTI_TILE_X(px) ((px & 0x07u) != 0x00u)
#define SPAN_MULTI_TILE_Y(py) ((py & 0x07u) != 0x00u)


uint16_t board_index;
uint8_t collide;

#define COL_UL 1
#define COL_UR 2
#define COL_LL 4
#define COL_LR 8


// uint8_t player_team_color;

void player_check_board_collisions_v2(uint8_t player_id, player_t * p_player) {

    // bool collision = false;
    // uint16_t board_index;
    // board_update_count = 0u;
    collide = 0u;

    uint8_t player_team_col = player_id & PLAYER_TEAMS_MASK;
    // Old method, look up color based on team (now just use player id directly)
    // uint8_t player_team_color = board_team_bg_colors[player_id & PLAYER_TEAMS_MASK];

    // player_t * p_player = &(gameinfo.players[player_id]);

    // Check for wall collisions
    // NOTE: Both checks rely on unsigned wraparound from 0
    // to also do the less than MIN test
#ifdef EMU_PRINT_ON
EMU_printf("  px= %d, py= %d", (uint16_t)p_player->next_x.h, (uint16_t)p_player->next_y.h);
#endif
        // Check Horizontal movement
        if (p_player->next_x.h > PLAYER_MAX_X_U8) {
            p_player->bounce_x = true;
#ifdef EMU_PRINT_ON
                EMU_printf("  WALL X: Bounce: id=%d, x = %d", (uint16_t)player_id, (uint16_t)p_player->next_x.h);
#endif
            return;
        } else
            p_player->bounce_x = false;

        // Check Vertical movement
        if (p_player->next_y.h > PLAYER_MAX_Y_U8) {
#ifdef EMU_PRINT_ON
                EMU_printf("  WALL Y: Bounce: id=%d, y = %d", (uint16_t)player_id, (uint16_t)p_player->next_y.h);
#endif
            p_player->bounce_y = true;
            return;
        } else
            p_player->bounce_y = false;


    // Check Board collisions
// return;

// TODO ==== FIX MISSING OUT OF BOUND TEST FOR BOARD BELOW ====
        // could just mask it out after calc?

    // Start in Upper-Left corner
    // uint8_t * p_board = gameinfo.board
        // EMU_printf(" *nx.h = %d, ny.h = %d", (uint16_t)p_player->next_x.h, p_player->next_y.h);
    board_index = (p_player->next_x.h / BOARD_GRID_SZ) + ((p_player->next_y.h / BOARD_GRID_SZ) * BOARD_BUF_W);


// *ptr style is 15,000 cycles slower on average in players_update

    // Check Upper-Right Corner
    if (SPAN_MULTI_TILE_X(p_player->next_x.h)) {

        if (SPAN_MULTI_TILE_Y(p_player->next_y.h)) {
            // All 4 corners touch dif`ferent tiles

            if (gameinfo.board[board_index] != player_team_col) {
                    gameinfo.board[board_index] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index, player_team_col);
                collide = COL_UL;
            }
            // Next tile column
            board_index++;`
        // EMU_printf(" 2.board_index = %d", (uint16_t)board_index);
            if (gameinfo.board[board_index] != player_team_col) {
                    gameinfo.board[board_index] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index, player_team_col);
                collide |= COL_UR;
            }

            // Next Tile Row on board (step back to start X)
            board_index += (BOARD_BUF_W - 1u);
        // EMU_printf(" 3.board_index = %d", (uint16_t)board_index);
            if (gameinfo.board[board_index] != player_team_col) {
                    gameinfo.board[board_index] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index, player_team_col);
                collide |= COL_LL;
            }
            // Next Tile column
            board_index++;`
        // EMU_printf(" 4.board_index = %d", (uint16_t)board_index);
            if (gameinfo.board[board_index] != player_team_col) {
                    gameinfo.board[board_index] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index, player_team_col);
                collide |= COL_LR;
            }
        // EMU_printf("collide = 0x%x", (uint16_t)collide);
#ifdef EMU_PRINT_ON
            if (collide) EMU_printf(" 2x2 collide = %x", (uint16_t)collide);
#endif
        }
        else {
            // Only Left/Right edges touch different tiles
            if (gameinfo.board[board_index] != player_team_col) {
                    gameinfo.board[board_index] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index, player_team_col);
                collide = COL_UL | COL_LL;
            }
            // Next Tile Column
            board_index++;
            if (gameinfo.board[board_index] != player_team_col) {
                collide |= COL_UR | COL_LR;
                gameinfo.board[board_index] = player_team_col;
                set_vram_byte(_SCRN0 + board_index, player_team_col);
            }
#ifdef EMU_PRINT_ON
            if (collide) EMU_printf(" 2x1 collide = %x", (uint16_t)collide);
#endif
        }
    }
    else {
        // X axis only touches one tile
        if (SPAN_MULTI_TILE_Y(p_player->next_y.h)) {
            // Only Top/Bottom edges touch different tiles
            if (gameinfo.board[board_index] != player_team_col) {
                    gameinfo.board[board_index] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index, player_team_col);
                collide = COL_UL | COL_UR;
            }

            // Next Tile Row on board
            board_index += BOARD_BUF_W;
            if (gameinfo.board[board_index] != player_team_col) {
                    gameinfo.board[board_index] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index, player_team_col);
                collide |= COL_LL | COL_LR;
            }
#ifdef EMU_PRINT_ON
            if (collide) EMU_printf(" 1x2 collide = %x", (uint16_t)collide);
#endif
        }
        else {
            // Ball only touches a single tile
            if (gameinfo.board[board_index] != player_team_col) {
                    gameinfo.board[board_index] = player_team_col;
                    set_vram_byte(_SCRN0 + board_index, player_team_col);
                collide = COL_UL | COL_UR | COL_LL | COL_LR;
            }
#ifdef EMU_PRINT_ON
            if (collide) EMU_printf(" 1x1 collide = %x", (uint16_t)collide);
#endif
        }
    }

// == BLEH THROW IT AWAY AND USE PREV METHOD ?

// THIS IS BETTER, But still looks OFF when a ball is mostly just colliding on one side
    // the old + new checking just "looks" better in the absensce of proper diagonal testing
    // since the sprites are actually square for collision purposes
    // It also MISSES wall+board collisions in corners (bounces but doesn't set the tiles?

// - also has problems with balls getting stuck on the edge of the screen
//    maybe more out of bounds problems?



    if (p_player->speed_x > 0) {
        // Right moving right
        switch (collide) {
            case (COL_UR                  ): // Corner only: ok
            case (COL_LR                  ):
            case (COL_UR | COL_LR         ): // Both Right: ok
            case (COL_UR | COL_LR | COL_UL): // Both Right + a diag: ok
            case (COL_UR | COL_LR | COL_LL):
            case (COL_UR          | COL_LL): // Right + a diagonal opposite: ok
            case (COL_LR          | COL_UL):
#ifdef EMU_PRINT_ON
                EMU_printf("  RIGHT: Bounce: id=%d, collid=%x", (uint16_t)player_id, (uint16_t)collide);
#endif
                p_player->bounce_x = true;
                break;
        }
        // if (collide & (COL_UR | COL_LR))
        //     p_player->bounce_x = true;
    }
    else if (p_player->speed_x < 0) {
        // Left edge moving left
        switch (collide) {
            case (COL_UL                  ): // Corner only: ok
            case (COL_LL                  ):
            case (COL_UL | COL_LL         ): // Both Left: ok
            case (COL_UL | COL_LL | COL_UR): // Both Left + a diag: ok
            case (COL_UL | COL_LL | COL_LR):
            case (COL_UL          | COL_LR): // Left + a diagonal opposite: ok
            case (COL_LL          | COL_UR):
#ifdef EMU_PRINT_ON
                EMU_printf("  LEFT: Bounce: id=%d, collid=%x", (uint16_t)player_id, (uint16_t)collide);
#endif
                p_player->bounce_x = true;
                break;
          }
        // if (collide & (COL_UL | COL_LL))
        //     p_player->bounce_x = true;
    }

    if (p_player->speed_y > 0) {
        // Bottom moving Down
        switch (collide) {
            case (COL_LL                  ): // Corner only: ok
            case (COL_LR                  ):
            case (COL_LL | COL_LR         ): // Both Bottom: ok
            case (COL_LL | COL_LR | COL_UL): // Both Bottom + a diag: ok
            case (COL_LL | COL_LR | COL_UR):
            case (COL_LL          | COL_UR): // Bottom + a diagonal opposite: ok
            case (COL_LR          | COL_UL):
#ifdef EMU_PRINT_ON
                EMU_printf("  UP: Bounce: id=%d, collid=%x", (uint16_t)player_id, (uint16_t)collide);
#endif
               p_player->bounce_y = true;
               break;
        }
        //  xx  if (collide & (COL_UL | COL_UR))
        //  xx   p_player->bounce_y = true;
    }
    else if (p_player->speed_y < 0) {
        // Top moving up
        switch (collide) {
            case (COL_UL                  ): // Corner only: ok
            case (COL_UR                  ):
            case (COL_UL | COL_UR         ): // Both Top: ok
            case (COL_UL | COL_UR | COL_LL): // Both Top + a diag: ok
            case (COL_UL | COL_UR | COL_LR):
            case (COL_UL          | COL_LR): // Top + a diagonal opposite: ok
            case (COL_UR          | COL_LL):
#ifdef EMU_PRINT_ON
                EMU_printf("  Down: Bounce: id=%d, collid=%x", (uint16_t)player_id, (uint16_t)collide);
#endif
               p_player->bounce_y = true;
               break;
        }
        // xx if (collide & (COL_LL | COL_LR))
        //  xx   p_player->bounce_y = true;
    }

    // if (p_player->speed_x > 0) {
    //     // Right moving right
    //     if (collide & (COL_UR | COL_LR))
    //         p_player->bounce_x = true;
    // }
    // else if (p_player->speed_x < 0) {
    //     // Left edge moving left
    //     if (collide & (COL_UL | COL_LL))
    //         p_player->bounce_x = true;
    // }

    // if (p_player->speed_y > 0) {
    //     // Top moving up
    //     if (collide & (COL_UL | COL_UR))
    //         p_player->bounce_y = true;
    // }
    // else if (p_player->speed_y < 0) {
    //     // Bottom moving left
    //     if (collide & (COL_LL | COL_LR))
    //         p_player->bounce_y = true;
    // }
}
*/

///////////////////////////////////////////



void player_check_board_collisions(uint8_t player_id) {

    board_update_count = 0;
    player_t * p_player = &(gameinfo.players[player_id]);

    uint8_t player_team_color = player_id & PLAYER_TEAMS_MASK;
    // Old method, look up color based on team (now just use player id directly)
    // uint8_t player_team_color = board_team_bg_colors[player_id & PLAYER_TEAMS_MASK];
    uint8_t nx       = p_player->next_x.h;
    uint8_t ny       = p_player->next_y.h;
    uint8_t px       = p_player->x.h;
    uint8_t py       = p_player->y.h;

    // Check Horizontal movement (New X & Current Y in order to avoid false triggers on Y)
    // Test separately here since collision check also updates board tile color
    if (p_player->speed_x != 0) {
        uint8_t test_x = (p_player->speed_x > 0) ? PLAYER_RIGHT(nx) : PLAYER_LEFT(nx);
        if (player_board_check_xy(test_x, PLAYER_TOP(py),    player_team_color)) p_player->bounce_x = true;
        if (player_board_check_xy(test_x, PLAYER_BOTTOM(py), player_team_color)) p_player->bounce_x = true;
    }

    // Check Vertical movement (New Y & Current X in order to avoid false triggers on X)
    // Test separately here since collision check also updates board tile color
    if (p_player->speed_y != 0) {
        uint8_t test_y = (p_player->speed_y > 0) ? PLAYER_BOTTOM(ny) : PLAYER_TOP(ny);
        if (player_board_check_xy(PLAYER_LEFT(px),  test_y, player_team_color)) p_player->bounce_y = true;
        if (player_board_check_xy(PLAYER_RIGHT(px), test_y, player_team_color)) p_player->bounce_y = true;
    }

    // Apply queued board updates
    while (board_update_count) {
        gameinfo.board[ board_update_queue[--board_update_count] ] = player_team_color;
    }
}


void player_check_wall_collisions(uint8_t player_id) {

    player_t * p_player = &(gameinfo.players[player_id]);

    // NOTE: Both checks rely on unsigned wraparound from 0
    // to also do the less than MIN test

    // Check Horizontal movement
    if (p_player->next_x.h > PLAYER_MAX_X_U8)
        p_player->bounce_x = true;
    else
        p_player->bounce_x = false;

    // Check Vertical movement
    if (p_player->next_y.h > PLAYER_MAX_Y_U8)
        p_player->bounce_y = true;
    else
        p_player->bounce_y = false;
}


uint8_t players_count;
void players_update(void) {


    players_count = gameinfo.player_count;
    player_t * p_player = &gameinfo.players[0];

    for (uint8_t c = 0; c < players_count; c++) {

        // Calculate next position (bounce flags get reset in test code)
        p_player->next_x.w = p_player->x.w + p_player->speed_x;
        p_player->next_y.w = p_player->y.w + p_player->speed_y;

        player_check_wall_collisions(c);
        player_check_board_collisions(c);

                // player_check_board_collisions_v2(c, p_player);
                // player_check_board_collisions_v3(c, p_player);
                // player_check_board_collisions_v4(c, p_player);

        // If there was a collision then calculate bounce angle
        // and don't update position
        if (p_player->bounce_x || p_player->bounce_y) {

            if (p_player->bounce_x)
                p_player->angle = (uint8_t)(ANGLE_TO_8BIT(360) - p_player->angle);

            if (p_player->bounce_y)
                p_player->angle = (uint8_t)(ANGLE_TO_8BIT(180) - p_player->angle);

            // Slightly perturb the player angle if there was a collision
            p_player->angle = (uint8_t)(p_player->angle + (int8_t)(rand() & 0x03u) - 1);
            player_recalc_movement(c);

        } else {
            // Otherwise update position to new location
            // Apply the new delta movement
            p_player->x.w = p_player->next_x.w;
            p_player->y.w = p_player->next_y.w;
        }
        p_player++;
    }
}


// 2x2 board layout with players at opposite angles
const uint8_t player_init_2x2_X[PLAYER_TEAMS_COUNT] = {
     (SCREENWIDTH / 4u) * 1u, (SCREENWIDTH / 4u) * 3u,
     (SCREENWIDTH / 4u) * 1u, (SCREENWIDTH / 4u) * 3u };

const uint8_t player_init_2x2_Y[PLAYER_TEAMS_COUNT] = {
     (SCREENHEIGHT / 4) * 1, (SCREENHEIGHT / 4) * 1,
     (SCREENHEIGHT / 4) * 3, (SCREENHEIGHT / 4) * 3 };

const uint8_t player_init_2x2_Angle[PLAYER_TEAMS_COUNT] = {
    ANGLE_TO_8BIT(45u), ANGLE_TO_8BIT(135u),
    ANGLE_TO_8BIT(315u), ANGLE_TO_8BIT(225u) };


void players_reset(void) {

    uint8_t c;

    // Always start with 2 x 2 layout
    for (c = 0; c < PLAYER_TEAMS_COUNT; c++) {
        gameinfo.players[c].x.h   = player_init_2x2_X[c];
        gameinfo.players[c].y.h   = player_init_2x2_Y[c];
        gameinfo.players[c].angle = player_init_2x2_Angle[c];
    }

    // Override first two if 2 x 1 layout
    if (gameinfo.player_count == PLAYERS_2_VAL) {
        gameinfo.players[0].x.h = (SCREENWIDTH / 4) * 1;
        gameinfo.players[1].x.h = (SCREENWIDTH / 4) * 3;
        // Centered on Y
        gameinfo.players[0].y.h = (SCREENHEIGHT / 2);
        gameinfo.players[1].y.h = (SCREENHEIGHT / 2);

        // Give players opposite angles
        gameinfo.players[0].angle = ANGLE_TO_8BIT(45u);
        gameinfo.players[1].angle = ANGLE_TO_8BIT(225u);
    }

    // Reset all player state vars
    for(uint8_t c = 0; c < PLAYER_COUNT_MAX; c++) {

        // Fill in any remaining player balls that weren't initialized above
        if (c >= PLAYER_TEAMS_COUNT) {
            // TODO: use algorithmic distribution with matching grid setup, alternating smaller X/Y division of regions per power of 2

            // More interesting to watch, but initially ugly due to being on random bg team colors, could fix up bg colors
            //
            // Random location roughly within board grid with angle opposite to same on team
            gameinfo.players[c].angle = player_init_2x2_Angle[c]; //(c & PLAYER_TEAMS_MASK) ^ PLAYER_TEAMS_MASK];
            // gameinfo.players[c].angle = rand();
            gameinfo.players[c].x.w   = (((rand() % PLAYER_RANGE_X_U8) + PLAYER_MIN_X_U8) << 8u);
            gameinfo.players[c].y.w   = (((rand() % PLAYER_RANGE_Y_U8) + PLAYER_MIN_Y_U8) << 8u);

            // Looks better initially but less interesting thereafter due to pre-clumping
            //
            // Match team locations for players 1-4, but with random angle
            // gameinfo.players[c].x.w   = gameinfo.players[c & PLAYER_TEAMS_MASK].x.w;
            // gameinfo.players[c].y.w   = gameinfo.players[c & PLAYER_TEAMS_MASK].y.w;
            // gameinfo.players[c].angle = rand();
        }

        gameinfo.players[c].next_x = gameinfo.players[c].x;
        gameinfo.players[c].next_y = gameinfo.players[c].y;
        gameinfo.players[c].bounce_x = gameinfo.players[c].bounce_y = false;
        // This will get recalculated when players_all_recalc_movement() is called before starting
        gameinfo.players[c].speed_x = gameinfo.players[c].speed_y = 0u;
        // Clear low byte of x/y position
        gameinfo.players[c].x.l = gameinfo.players[c].y.l = 0u;
    }
}
