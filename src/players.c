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


// offset to center the 8X8 circle sprite on the x,y coordinate
#define SPR_OFFSET_X (DEVICE_SPRITE_PX_OFFSET_X / 2)
#define SPR_OFFSET_Y ((DEVICE_SPRITE_PX_OFFSET_Y / 2) + (DEVICE_SPRITE_PX_OFFSET_Y / 4))



// Player corner calculations (Sprite is 8 pixels wide, so from Center: Left/Top = -4, Right/Bottom = +3)
#define PLAYER_LEFT(px)   ((px - ((SPRITE_WIDTH  / 2)    )) / BOARD_GRID_SZ)
#define PLAYER_RIGHT(px)  ((px + ((SPRITE_WIDTH  / 2) - 1)) / BOARD_GRID_SZ)
#define PLAYER_TOP(py)    ((py - ((SPRITE_HEIGHT / 2)    )) / BOARD_GRID_SZ)
#define PLAYER_BOTTOM(py) ((py + ((SPRITE_HEIGHT / 2) - 1)) / BOARD_GRID_SZ)


uint16_t board_update_queue[PLAYER_TEAMS_COUNT];
uint8_t board_update_count;

// ====== PLAYERS ======

void players_redraw_sprites(void) {
    for (uint8_t c = 0; c < gameinfo.player_count; c++) {
        move_sprite(c, gameinfo.players[c].x.h + SPR_OFFSET_X,
                       gameinfo.players[c].y.h + SPR_OFFSET_Y);
    }
}


// Recalc X,Y speed based on Angle and Speed for a given player
void player_recalc_movement(uint8_t idx) {
    uint8_t t_angle = gameinfo.players[idx].angle;

    gameinfo.players[idx].speed_x = (int16_t)SIN(t_angle) * gameinfo.speed;
    // Flip Y direction since adding positive values moves further down the screen
    gameinfo.players[idx].speed_y = (int16_t)COS(t_angle) * gameinfo.speed * -1;
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
    if ((x >= BOARD_W) || (y >= BOARD_H)) {
         // Return false here since it's off the grid
         // It's ok to do that since the player sprite is not bigger than a tile
         // so if one edge is off a remaining corner will still get checked at the right location
        return false;
    }

    uint16_t board_index = x + (y * BOARD_W);
    if (gameinfo.board[board_index] != player_team_col) {
        // Don't update the board itself here since it needs to remain unmodified
        // until both Horizontal and Vertical testing is done, so queue an update
        board_update_queue[board_update_count++] = board_index;
        // OPTIONAL: queue a tile draw instead of handling immediately
        set_bkg_tile_xy(x, y, player_team_col);
        collision = true;
    }

    return collision;
}


void player_check_board_collisions(uint8_t player_id) {

    board_update_count = 0;
    player_t * p_player = &(gameinfo.players[player_id]);

    // TODO: OPTIMIZE: Remove the array lookup out and just use a bitmask on the player id, Re-arrange colors so board_team_bg_colors can be dropped
    uint8_t player_team_color = board_team_bg_colors[player_id & PLAYER_TEAMS_MASK];
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


// Check for collision with BG Tile and modify it
//
void player_check_wall_collisions(uint8_t player_id) {

    bool movement_recalc_queued = false;
    player_t * p_player = &(gameinfo.players[player_id]);

    // Check Horizontal movement
    if ((p_player->next_x.w < PLAYER_MIN_X_U16) || (p_player->next_x.w > PLAYER_MAX_X_U16))
        p_player->bounce_x = true;

    // Check Vertical movement
    if ((p_player->next_y.w < PLAYER_MIN_Y_U16) || (p_player->next_y.w > PLAYER_MAX_Y_U16))
        p_player->bounce_y = true;
}


void players_update(void) {

    player_t * p_player;

    for (uint8_t c = 0; c < gameinfo.player_count; c++) {

        p_player = &(gameinfo.players[c]);

        // Calculate next position and reset flags
        p_player->bounce_x = p_player->bounce_y = false;
        p_player->next_x.w = p_player->x.w + p_player->speed_x;
        p_player->next_y.w = p_player->y.w + p_player->speed_y;

        player_check_wall_collisions(c);
        player_check_board_collisions(c);

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
    }

    if (gameinfo.sprites_enabled) players_redraw_sprites();
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
            gameinfo.players[c].x.w   = ((rand() % PLAYER_RANGE_X_U8) << 8u) + PLAYER_MIN_X_U16;
            gameinfo.players[c].y.w   = ((rand() % PLAYER_RANGE_Y_U8) << 8u) + PLAYER_MIN_Y_U16;

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
