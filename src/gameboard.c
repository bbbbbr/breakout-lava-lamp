#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
#include <gbdk/emu_debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <rand.h>

#include "common.h"
#include "input.h"
#include "gfx.h"
#include "math_util.h"

#include "gameboard.h"


const uint8_t board_player_colors[PLAYER_COUNT_MAX] = {
    BOARD_COL_0, BOARD_COL_1, BOARD_COL_2, BOARD_COL_3
};

const uint8_t player_colors[PLAYER_COUNT_MAX] = {
    PLAYER_COL_0, PLAYER_COL_1,
    PLAYER_COL_0, PLAYER_COL_1
};


// offset to center the 8X8 circle sprite on the x,y coordinate
#define SPR_OFFSET_X (DEVICE_SPRITE_PX_OFFSET_X / 2)
#define SPR_OFFSET_Y ((DEVICE_SPRITE_PX_OFFSET_Y / 2) + (DEVICE_SPRITE_PX_OFFSET_Y / 4))

// ====== PLAYERS ======


static void players_redraw_sprites(void) {
    for (uint8_t c = 0; c < gameinfo.player_count; c++) {
//        if (c == 0)
        move_sprite(c, gameinfo.players[c].x.h + SPR_OFFSET_X,
                       gameinfo.players[c].y.h + SPR_OFFSET_Y);
// if (c == 0)
    // EMU_printf("x=%d, y=%d", gameinfo.players[c].x.h + SPR_OFFSET_X, gameinfo.players[c].y.h + SPR_OFFSET_Y);
    }
}

static void player_update_direction(uint8_t idx) {
    uint8_t t_angle = gameinfo.players[idx].angle;

    gameinfo.players[idx].speed_x = (int16_t)SIN(t_angle) * PLAYER_SPEED_DEFAULT;
    // Need to flip Y direction since adding positive values moves further down the screen
    gameinfo.players[idx].speed_y = (int16_t)COS(t_angle) * PLAYER_SPEED_DEFAULT * -1;
}


// TODO: make a board redraw queue for end of frame instead of redraws inline

// TODO: Still buggy with edge overflow checks
// TODO: simplify to array of offsets and L/Rvs U/D and a helper function

static bool player_check_board_collisions(uint8_t player_id) {
        // TODO: Optimize: If going left, just check left edge ?
        bool movement_recalc_queued = false;
        bool bounce_x = false;
        bool bounce_y = false;

        player_t * p_player = &(gameinfo.players[player_id]);

        uint8_t board_player_col = board_player_colors[player_id];

        // uint8_t px       = (p_player->x.w + p_player->speed_x) >> 8;
        uint8_t px       = p_player->x.h;
        uint8_t x_mid    = px / BOARD_GRID_SZ;
        uint8_t x_left   = (px - (SPRITE_WIDTH  / 2)) / BOARD_GRID_SZ;
        uint8_t x_right  = (px + (SPRITE_WIDTH  / 2)) / BOARD_GRID_SZ;

        // uint8_t py       = (p_player->y.w + p_player->speed_y) >> 8;
        uint8_t py       = p_player->y.h;
        uint8_t y_mid    = py / BOARD_GRID_SZ;
        uint8_t y_top    = (py - (SPRITE_HEIGHT / 2)) / BOARD_GRID_SZ;
        uint8_t y_bottom = (py + (SPRITE_HEIGHT / 2)) / BOARD_GRID_SZ;

        uint16_t board_check_xy;

        uint8_t test_x;
        uint8_t test_y;

        // ================ Test X

        if (p_player->speed_x > 0)
            test_x = x_right;
        else
            test_x = x_left;

        test_y = y_top;
        board_check_xy = test_x + (test_y * BOARD_W);
if (board_check_xy > BOARD_W * BOARD_H)
EMU_printf("OFLOW-X-TOP px=%d, py=%d, tx=%d, ty=%d, checkxy=%d", (int16_t)px, (int16_t)py, (int16_t)test_x, (int16_t)test_y, (int16_t)board_check_xy);
        //EMU_printf("px=%d, py=%d, tx=%d, ty=%d, checkxy=%d", (int16_t)px, (int16_t)py, (int16_t)test_x, (int16_t)test_y, (int16_t)board_check_xy);
        if (gameinfo.board[board_check_xy] != board_player_col) {
            // Update board
            gameinfo.board[board_check_xy] = board_player_col;
            set_bkg_tile_xy(test_x, test_y, board_player_col);
            bounce_x = true;
        }

        test_y = y_bottom;
        board_check_xy = test_x + (test_y * BOARD_W);
if (board_check_xy > BOARD_W * BOARD_H)
EMU_printf("OFLOW-X-BOTTOM px=%d, py=%d, tx=%d, ty=%d, checkxy=%d", (int16_t)px, (int16_t)py, (int16_t)test_x, (int16_t)test_y, (int16_t)board_check_xy);
        if (gameinfo.board[board_check_xy] != board_player_col) {
            // Update board
            gameinfo.board[board_check_xy] = board_player_col;
            set_bkg_tile_xy(test_x, test_y, board_player_col);
            bounce_x = true;
        }

        if (bounce_x) {
            p_player->angle = (uint8_t)(ANGLE_TO_8BIT(360) - p_player->angle);
            movement_recalc_queued = true;
        }


    // ================ Test Y

    if (p_player->speed_y > 0)
        test_y = y_bottom;
    else
        test_y = y_top;

    test_x = x_left;
    board_check_xy = test_x + (test_y * BOARD_W);
if (board_check_xy > BOARD_W * BOARD_H)
EMU_printf("OFLOW-Y-LEFT px=%d, py=%d, tx=%d, ty=%d, checkxy=%d", (int16_t)px, (int16_t)py, (int16_t)test_x, (int16_t)test_y, (int16_t)board_check_xy);
    //EMU_printf("px=%d, py=%d, tx=%d, ty=%d, checkxy=%d", (int16_t)px, (int16_t)py, (int16_t)test_x, (int16_t)test_y, (int16_t)board_check_xy);
    if (gameinfo.board[board_check_xy] != board_player_col) {
        // Update board
        gameinfo.board[board_check_xy] = board_player_col;
        set_bkg_tile_xy(test_x, test_y, board_player_col);
        bounce_y = true;
    }

    test_x = x_right;
    board_check_xy = test_x + (test_y * BOARD_W);
if (board_check_xy > BOARD_W * BOARD_H)
EMU_printf("OFLOW-Y-RIGHT px=%d, py=%d, tx=%d, ty=%d, checkxy=%d", (int16_t)px, (int16_t)py, (int16_t)test_x, (int16_t)test_y, (int16_t)board_check_xy);
    if (gameinfo.board[board_check_xy] != board_player_col) {
        // Update board
        gameinfo.board[board_check_xy] = board_player_col;
        set_bkg_tile_xy(test_x, test_y, board_player_col);
        bounce_y = true;
    }

    if (bounce_y) {
        p_player->angle = (uint8_t)(ANGLE_TO_8BIT(180) - p_player->angle);
        movement_recalc_queued = true;
    }


    return movement_recalc_queued;
}


// TODO: can walls be made into just part of the grid that doesn't erase?
//
// Check for collision with BG Tile and modify it
//
static bool player_check_wall_collisions(uint8_t player_id) {

    bool movement_recalc_queued = false;
    player_t * p_player = &(gameinfo.players[player_id]);

    // X (horizontal)
    if ( ((p_player->x.w + p_player->speed_x) < PLAYER_MIN_X_U16) ||
         ((p_player->x.w + p_player->speed_x) > PLAYER_MAX_X_U16) ) {
        // p_player->speed_x *= -1;  // Simple version
        p_player->angle = (uint8_t)(ANGLE_TO_8BIT(360) - p_player->angle);
        movement_recalc_queued = true;
    }
    // Y (vertical)
    if ( ((p_player->y.w + p_player->speed_y) < PLAYER_MIN_Y_U16) ||
         ((p_player->y.w + p_player->speed_y) > PLAYER_MAX_Y_U16) ) {
        // p_player->speed_y *= -1;  // Simple version
        p_player->angle = (uint8_t)(ANGLE_TO_8BIT(180) - p_player->angle);

        movement_recalc_queued = true;
    }

    return movement_recalc_queued;
}


static void players_update(void) {

    player_t * p_player;
    bool movement_recalc_queued;


    // for (uint8_t c = 0; c < 1; c++) {
    for (uint8_t c = 0; c < gameinfo.player_count; c++) {
        movement_recalc_queued = false;

        if (player_check_wall_collisions(c))
            movement_recalc_queued = true;

        if (player_check_board_collisions(c))
            movement_recalc_queued = true;

        // == Apply the position updates
        p_player = &(gameinfo.players[c]);

        if (movement_recalc_queued == true) {

            // If there was a collision slightly perturb the player angle, but not often
            // if ((rand() & 0x0Fu) == 0x00) {
                // Angle will be one of: -1,0,1
                p_player->angle = (uint8_t)(p_player->angle + (int8_t)(rand() & 0x03u) - 1);
            // }

            player_update_direction(c);
        }

        // Apply the new delta movement
        p_player->x.w += p_player->speed_x;
        p_player->y.w += p_player->speed_y;
    }

    players_redraw_sprites();
}


static void players_init(void) {

    if (gameinfo.player_count == PLAYER_COUNT_4) {

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
    else { // Default, implied: PLAYER_COUNT_2

        gameinfo.players[0].x.h = (SCREENWIDTH / 4) * 1;
        gameinfo.players[1].x.h = (SCREENWIDTH / 4) * 3;
        // Centered on Y
        gameinfo.players[0].y.h = (SCREENHEIGHT / 2);
        gameinfo.players[1].y.h = (SCREENHEIGHT / 2);

        // Give players opposite angles
        gameinfo.players[0].angle = ANGLE_TO_8BIT(45u);
        gameinfo.players[1].angle = ANGLE_TO_8BIT(225u);
    }

    // Calculate default speed based on angles and uniform speed
    for (uint8_t c = 0; c < gameinfo.player_count; c++) {
        player_update_direction(c);
    }
}

// ====== BOARD ======


// Initialize the game board grid to 2 or 4 regions
static void board_init_grid(void) {

    uint8_t * p_board = gameinfo.board;
    for (uint8_t y = 0; y < BOARD_H; y++) {
        for (uint8_t x = 0; x < BOARD_W; x++) {

            if (gameinfo.player_count == PLAYER_COUNT_4) {

                // Divide board into 4 color regions, Left & Right and Top & Bottom
                *p_board = board_player_colors[ (x / (BOARD_W / 2)) + ((y / (BOARD_H / 2)) * 2u) ];
            }
            else { // Default, implied: PLAYER_COUNT_2

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

    hide_sprites_range(0u, MAX_HARDWARE_SPRITES);

    for (uint8_t c = 0; c < gameinfo.player_count; c++) {
        set_sprite_tile(c, player_colors[c]);
    }

    players_redraw_sprites();
}


void board_init(void) {

    players_init();
    board_init_grid();
    board_init_gfx();
}


void board_run(void) {

    while(TRUE) {

        players_update();
        vsync();
    }
}