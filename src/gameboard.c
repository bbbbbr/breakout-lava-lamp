#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
#include <gbdk/emu_debug.h>
#include <rand.h>

#include "common.h"
#include "input.h"
#include "gfx.h"
#include "math_util.h"

#include "gameboard.h"

// Background colors for the board (2x1 and 2x2 style)
const uint8_t board_player_colors[PLAYER_COUNT_MAX] = {
    BOARD_COL_WHITE,  BOARD_COL_BLACK,
    BOARD_COL_D_GREY, BOARD_COL_L_GREY
};

// Sprite colors for the player ball (2x1 and 2x2 style)
const uint8_t player_colors[PLAYER_COUNT_MAX] = {
    PLAYER_COL_BLACK, PLAYER_COL_WHITE,
    PLAYER_COL_WHITE, PLAYER_COL_BLACK
};


// offset to center the 8X8 circle sprite on the x,y coordinate
#define SPR_OFFSET_X (DEVICE_SPRITE_PX_OFFSET_X / 2)
#define SPR_OFFSET_Y ((DEVICE_SPRITE_PX_OFFSET_Y / 2) + (DEVICE_SPRITE_PX_OFFSET_Y / 4))



// Player corner calculations (Sprite is 8 pixels wide, so from Center: Left/Top = -4, Right/Bottom = +3)
#define PLAYER_LEFT(px)   ((px - ((SPRITE_WIDTH  / 2)    )) / BOARD_GRID_SZ)
#define PLAYER_RIGHT(px)  ((px + ((SPRITE_WIDTH  / 2) - 1)) / BOARD_GRID_SZ)
#define PLAYER_TOP(py)    ((py - ((SPRITE_HEIGHT / 2)    )) / BOARD_GRID_SZ)
#define PLAYER_BOTTOM(py) ((py + ((SPRITE_HEIGHT / 2) - 1)) / BOARD_GRID_SZ)


uint16_t board_update_queue[4];
uint8_t board_update_count;

// ====== PLAYERS ======

static void players_redraw_sprites(void) {
    for (uint8_t c = 0; c < gameinfo.player_count; c++) {
        move_sprite(c, gameinfo.players[c].x.h + SPR_OFFSET_X,
                       gameinfo.players[c].y.h + SPR_OFFSET_Y);
    }
}


// Recalc X,Y speed based on Angle and Speed for a given player
static void player_recalc_movement(uint8_t idx) {
    uint8_t t_angle = gameinfo.players[idx].angle;

    gameinfo.players[idx].speed_x = (int16_t)SIN(t_angle) * gameinfo.speed;
    // Flip Y direction since adding positive values moves further down the screen
    gameinfo.players[idx].speed_y = (int16_t)COS(t_angle) * gameinfo.speed * -1;
}


// Recalc X,Y speed based on Angle and Speed for all players
static void players_all_recalc_movement(void) {
        // Calculate default speed based on angles and uniform speed
    for (uint8_t c = 0; c < gameinfo.player_count; c++) {
        player_recalc_movement(c);
    }
}


// Check to see if a given board tile does not match the player (collision) or matches (no collision)
// OPTIMIZE: could inline to improve performance
static bool board_check_xy(uint8_t x, uint8_t y, uint8_t board_player_col) {
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
    if (gameinfo.board[board_index] != board_player_col) {
        // Don't update the board itself here since it needs to remain unmodified
        // until both Horizontal and Vertical testing is done, so queue an update
        board_update_queue[board_update_count++] = board_index;
        // OPTIONAL: queue a tile draw instead of handling immediately
        set_bkg_tile_xy(x, y, board_player_col);
        collision = true;
    }

    return collision;
}


static void player_check_board_collisions(uint8_t player_id) {

    board_update_count = 0;
    player_t * p_player = &(gameinfo.players[player_id]);

    uint8_t board_player_col = board_player_colors[player_id];
    uint8_t nx       = p_player->next_x.h;
    uint8_t ny       = p_player->next_y.h;
    uint8_t px       = p_player->x.h;
    uint8_t py       = p_player->y.h;

    // Check Horizontal movement (New X & Current Y in order to avoid false triggers on Y)
    // Test separately here since collision check also updates board tile color
    if (p_player->speed_x != 0) {
        uint8_t test_x = (p_player->speed_x > 0) ? PLAYER_RIGHT(nx) : PLAYER_LEFT(nx);
        if (board_check_xy(test_x, PLAYER_TOP(py),    board_player_col)) p_player->bounce_x = true;
        if (board_check_xy(test_x, PLAYER_BOTTOM(py), board_player_col)) p_player->bounce_x = true;
    }

    // Check Vertical movement (New Y & Current X in order to avoid false triggers on X)
    // Test separately here since collision check also updates board tile color
    if (p_player->speed_y != 0) {
        uint8_t test_y = (p_player->speed_y > 0) ? PLAYER_BOTTOM(ny) : PLAYER_TOP(ny);
        if (board_check_xy(PLAYER_LEFT(px),  test_y, board_player_col)) p_player->bounce_y = true;
        if (board_check_xy(PLAYER_RIGHT(px), test_y, board_player_col)) p_player->bounce_y = true;
    }

    // Apply queued board updates
    while (board_update_count) {
        gameinfo.board[ board_update_queue[--board_update_count] ] = board_player_col;
    }
}


// Check for collision with BG Tile and modify it
//
static void player_check_wall_collisions(uint8_t player_id) {

    bool movement_recalc_queued = false;
    player_t * p_player = &(gameinfo.players[player_id]);

    // Check Horizontal movement
    if ((p_player->next_x.w < PLAYER_MIN_X_U16) || (p_player->next_x.w > PLAYER_MAX_X_U16))
        p_player->bounce_x = true;

    // Check Vertical movement
    if ((p_player->next_y.w < PLAYER_MIN_Y_U16) || (p_player->next_y.w > PLAYER_MAX_Y_U16))
        p_player->bounce_y = true;
}


static void players_update(void) {

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

    players_redraw_sprites();
}


static void players_reset(void) {

    // 2x2 layout
    if (gameinfo.player_count == PLAYERS_4_VAL) {

        // Place players centered in their respective regions
        // Left/Right
        gameinfo.players[0].x.h = (SCREENWIDTH / 4) * 1;
        gameinfo.players[1].x.h = (SCREENWIDTH / 4) * 3;
        // Left/Right
        gameinfo.players[2].x.h = (SCREENWIDTH / 4) * 1;
        gameinfo.players[3].x.h = (SCREENWIDTH / 4) * 3;

        // Top
        gameinfo.players[0].y.h = (SCREENHEIGHT / 4) * 1;
        gameinfo.players[1].y.h = (SCREENHEIGHT / 4) * 1;
        // Bottom
        gameinfo.players[2].y.h = (SCREENHEIGHT / 4) * 3;
        gameinfo.players[3].y.h = (SCREENHEIGHT / 4) * 3;

        // Give players opposite angles
        gameinfo.players[0].angle = ANGLE_TO_8BIT(45u);
        gameinfo.players[1].angle = ANGLE_TO_8BIT(135u);
        gameinfo.players[2].angle = ANGLE_TO_8BIT(315u);
        gameinfo.players[3].angle = ANGLE_TO_8BIT(225u);

    }
    else { // Default, implied: PLAYERS_2_VAL

        // 2 x 1 layout

        gameinfo.players[0].x.h = (SCREENWIDTH / 4) * 1;
        gameinfo.players[1].x.h = (SCREENWIDTH / 4) * 3;
        // Centered on Y
        gameinfo.players[0].y.h = (SCREENHEIGHT / 2);
        gameinfo.players[1].y.h = (SCREENHEIGHT / 2);

        // Give players opposite angles
        gameinfo.players[0].angle = ANGLE_TO_8BIT(45u);
        gameinfo.players[1].angle = ANGLE_TO_8BIT(225u);
    }

    // Reset remaining player state vars
    for(uint8_t c = 0; c < gameinfo.player_count; c++) {
        gameinfo.players[c].next_x = gameinfo.players[c].x;
        gameinfo.players[c].next_y = gameinfo.players[c].y;
        gameinfo.players[c].bounce_x = gameinfo.players[c].bounce_y = false;
        // This will get recalculated when players_all_recalc_movement() is called before starting
        gameinfo.players[c].speed_x = gameinfo.players[c].speed_y = 0u;

        gameinfo.players[c].x.l = gameinfo.players[c].y.l = 0u;
    }
}

// ====== BOARD ======


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
        set_sprite_tile(c, player_colors[c]);
    }

    players_redraw_sprites();
}


// Reset Board and Players states
// Not called when "Continue" Title Menu action is used, unless the number of players was changed
void board_reset(void) {

    // TODO: wait for non-deterministic user input to seed random numbers
    initrand(0x1234u);

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

    while(TRUE) {
        UPDATE_KEYS();
        players_update();

        // Skip Vsync while SELECT is pressed
        if (!(KEY_PRESSED(J_SELECT)))
            vsync();

        // Return to Title menu if some keys pressed
        if (KEY_TICKED(J_START | J_A | J_B))
            return;
    }
}