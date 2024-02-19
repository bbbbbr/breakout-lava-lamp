#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
#include <gbdk/emu_debug.h>
#include <rand.h>

#include "common.h"
#include "input.h"
#include "gfx.h"
#include "math_util.h"
#include "save_and_restore.h"

#include "title_screen.h"
#include "gameboard.h"
#include "players_asm.h"

// #define EMU_PRINT_ON

// offset to center the 8X8 circle sprite on the x,y coordinate
#define SPR_OFFSET_X (DEVICE_SPRITE_PX_OFFSET_X)
#define SPR_OFFSET_Y (DEVICE_SPRITE_PX_OFFSET_Y)


// // 2x2 board layout with players at opposite angles
// const uint8_t player_init_2x2_X[PLAYER_TEAMS_COUNT] = {
//      (SCREENWIDTH / 4u) * 1u, (SCREENWIDTH / 4u) * 3u,
//      (SCREENWIDTH / 4u) * 1u, (SCREENWIDTH / 4u) * 3u };

// const uint8_t player_init_2x2_Y[PLAYER_TEAMS_COUNT] = {
//      (SCREENHEIGHT / 4) * 1, (SCREENHEIGHT / 4) * 1,
//      (SCREENHEIGHT / 4) * 3, (SCREENHEIGHT / 4) * 3 };

const uint8_t player_init_2x2_Angle[PLAYER_TEAMS_COUNT] = {
    ANGLE_TO_8BIT(45u), ANGLE_TO_8BIT(135u),
    ANGLE_TO_8BIT(315u), ANGLE_TO_8BIT(225u) };



// Player corner calculations (Sprite is 8 pixels wide, Upper Left is 0,0, Lower Right is +7,+7)
#define PLAYER_LEFT(px)   ((px                     ) / BOARD_GRID_SZ)
#define PLAYER_RIGHT(px)  ((px + (SPRITE_WIDTH - 1)) / BOARD_GRID_SZ)
#define PLAYER_TOP(py)    ((py                     ) / BOARD_GRID_SZ)
#define PLAYER_BOTTOM(py) ((py + (SPRITE_HEIGHT - 1)) / BOARD_GRID_SZ)

// Absolute worst case update queue scenario is each ball spans 4 separate tiles (2x2)
// In practice max is more like ~115 when starting out with 32 balls, so some RAM is wasted
#define UPDATE_QUEUE_MAX_SZ  (PLAYER_COUNT_MAX  * 4)
uint16_t board_update_queue[UPDATE_QUEUE_MAX_SZ];
uint8_t g_board_update_count;
uint8_t g_board_update_count_cur_player;

// Global Player update loop vars
// Resulting codegen is faster with a global than stack local or static
uint16_t g_board_index;
uint8_t g_players_count;
uint8_t g_collisions;

#define COLLIDE_NONE  0x00u
#define COLLIDE_X     0x01u
#define COLLIDE_Y     0x02u

// Checks whether a given upper/left pixel coordinate spans more than 1 tile
// (i.e is not evenly aligned on grid boundary with x % 8 == 0)
#define CHECK_BOARD_SPAN_2_TILES(pixel_loc) (pixel_loc & 0x07u)



void players_redraw_sprites_asm(void) NAKED PRESERVES_REGS(a, b, c, d, e, h, l) {
      __asm \

    _GINFO_OFSET_PLAYER_COUNT = 6  // ; location of (uint8_t) .player_count
    _GINFO_OFSET_PLAYERS      = 17 // ; start of (player_t) .players[N]
    // ; Offset indexes into the player struct
    _PLAYER_TYPE_Y_HI_OFFSET      = 2 // ; .y.h
    _PLAYER_TYPE_X_HI_OFFSET      = 8 // ; .x.h  // This relies on even distance between paired X/Y vars in the struct in each direction


    _PLAYER_TYPE_Y_TO_X_INCR      = (_PLAYER_TYPE_X_HI_OFFSET - _PLAYER_TYPE_Y_HI_OFFSET)

    push af
    push hl
    push bc
    push de

._redraw_sprites_loop_init:
    // == Set up Counter for players ==
    // ; players_count = gameinfo.player_count;
    ld  a, (#_gameinfo + #_GINFO_OFSET_PLAYER_COUNT) // ; Counter max: gameinfo.player_count
    ld  d, a

    ld  bc, #_shadow_OAM
    // ; Start at player[0].y.h
    ld  hl, #_gameinfo + #(_GINFO_OFSET_PLAYERS  + _PLAYER_TYPE_Y_HI_OFFSET) // ; &gameinfo.players[0].y.h
    ld  e, #_PLAYER_TYPE_Y_TO_X_INCR

    ._redraw_sprites_loop:

        // ; Copy sprite Y position with offset
        ld  a, (hl)
        add a, #DEVICE_SPRITE_PX_OFFSET_Y
        ld  (bc), a

        // ; Advance to X OAM slot
        inc bc

        // Advance to Player X position
        ld  a, d
        ld  d, #0
        add hl, de
        ld  d, a

        // ; Copy sprite X position with offset
        ld  a, (hl+)  // ; HL increment to allow reuse of E for HL incrementing to next player (since it's +8 instead of +7)
        add a, #DEVICE_SPRITE_PX_OFFSET_X
        ld  (bc), a

        // ; Advance to start of next OAM slot
        inc bc
        inc bc
        inc bc

        // Advance to next player[n].y.h
        ld  a, d
        ld  d, #0
        add hl, de
        ld  d, a

        dec d
        jr  NZ, ._redraw_sprites_loop

    pop de
    pop bc
    pop hl
    pop af

    ret
   __endasm;
}


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

    gameinfo.players[idx].speed_x = (int16_t)SIN_PREMULT(t_angle);
    // Flip Y direction since adding positive values moves further down the screen
    gameinfo.players[idx].speed_y = (int16_t)COS_PREMULT(t_angle) * -1;
}


// Recalc X,Y speed based on Angle and Speed for all players
void players_all_recalc_movement(void) {
        // Calculate default speed based on angles and uniform speed
    for (uint8_t c = 0; c < gameinfo.player_count; c++) {
        player_recalc_movement(c);
    }
}


inline uint8_t players_check_wall_collisions(uint8_t collisions, player_t * p_player) {

    // Check Horizontal wall bounces via out-of-bounds
    if (p_player->next_x.h > PLAYER_MAX_X_U8) {
        collisions = COLLIDE_X;

        // Clamp values so later array lookups aren't garbage
        // Left edge wraparound will be higher, otherwise it's Right edge
        if (p_player->next_x.h > PLAYER_MAX_X_U8 + 8)
            p_player->next_x.w = 0;
        else 
            p_player->next_x.w = PLAYER_MAX_X_U8 << 8;                
    }
    else
        collisions = COLLIDE_NONE;

    // Check Vertical movement
    if (p_player->next_y.h > PLAYER_MAX_Y_U8) {
        collisions |= COLLIDE_Y;

        // Clamp values so later array lookups aren't garbage
        // Left edge wraparound will be higher, otherwise it's Right edge
        if (p_player->next_y.h > PLAYER_MAX_Y_U8 + 8)
            p_player->next_y.w = 0;
        else 
            p_player->next_y.w = PLAYER_MAX_Y_U8 << 8;
    }

    return collisions;
}


bool dummy_always_true = true;

// Faster codegen when collisions is passed in as a local
// even though it's an inline function
inline uint8_t players_check_board_collisions(uint8_t collisions, uint8_t player_team_color, player_t * p_player) {

    // Test separately here even if there was a wall collision
    // since board collision check also updates board tile color which might
    // change underneath the player due to other players
    // if ((p_player->speed_x != 0) && (!(collisions & COLLIDE_X)) ) {
    // if ((p_player->speed_y != 0) && (!(collisions & COLLIDE_X)) ){

    // Check Horizontal movement (New X & Current Y in order to avoid false triggers on Y)
    //
    // WARNING: Something about the optimizer requires the dummy test, 
    // otherwise players_update is ~40,000 cycles slower with 32 balls. 
    if ((p_player->speed_x != 0) && (dummy_always_true)) {

        // Faster with the stack local vars instead of a compounded statement with index calc
        uint8_t test_next_x = (p_player->speed_x > 0) ? PLAYER_RIGHT(p_player->next_x.h) : PLAYER_LEFT(p_player->next_x.h);
        uint8_t test_y = PLAYER_TOP(p_player->y.h);
        g_board_index = (test_next_x) + ((test_y) * BOARD_BUF_W);

       if (gameinfo.board[g_board_index] != player_team_color) {
            // Don't update the board itself here since it needs to remain unmodified
            // until both Horizontal and Vertical testing is done, so queue an update
            board_update_queue[g_board_update_count++] = g_board_index;
            collisions |= COLLIDE_X;
        }

        if (CHECK_BOARD_SPAN_2_TILES(p_player->y.h)) {
            // OK, need to check next Tile Row on board
            g_board_index += BOARD_BUF_W;

           if (gameinfo.board[g_board_index] != player_team_color) {
                board_update_queue[g_board_update_count++] = g_board_index;
                collisions |= COLLIDE_X;
            }
        }
    }

    // Check Vertical movement (New Y & Current X in order to avoid false triggers on X)
    //
    // WARNING: Something about the optimizer requires the dummy test, 
    // otherwise players_update is ~40,000 cycles slower with 32 balls. 
    if ((p_player->speed_y != 0) && (dummy_always_true)) {

        uint8_t test_x = PLAYER_LEFT(p_player->x.h);
        uint8_t test_next_y = (p_player->speed_y > 0) ? PLAYER_BOTTOM(p_player->next_y.h) : PLAYER_TOP(p_player->next_y.h);

        g_board_index = ( test_x) + (( test_next_y) * BOARD_BUF_W);

       if (gameinfo.board[g_board_index] != player_team_color) {
            // Don't update the board itself here since it needs to remain unmodified
            // until both Horizontal and Vertical testing is done, so queue an update
            board_update_queue[g_board_update_count++] = g_board_index;
            collisions |= COLLIDE_Y;
        }

        if (CHECK_BOARD_SPAN_2_TILES(p_player->x.h)) {
            // OK, need to check next Tile Column on board
            g_board_index ++;

           if (gameinfo.board[g_board_index] != player_team_color) {
                board_update_queue[g_board_update_count++] = g_board_index;
                collisions |= COLLIDE_Y;
            }
        }
    }

    return collisions;
}


// Loop through all players, update their position, test for
// wall and board collisions and update their bounce angle/etc if needed
void players_update(void) {

    g_players_count = gameinfo.player_count;
    player_t * p_player = &gameinfo.players[0];

    g_board_update_count = 0;
    for (uint8_t c = 0; c < g_players_count; c++) {

        // Create local offset for current ball index in update queue
        // This allows applying board updates right away but defering
        // vram updates until after frame calc is done
        g_board_update_count_cur_player = g_board_update_count;

        uint8_t player_team_color = c & PLAYER_TEAMS_MASK;

        // Calculate next position (bounce flags get reset in test code)
        p_player->next_x.w = p_player->x.w + p_player->speed_x;
        p_player->next_y.w = p_player->y.w + p_player->speed_y;


        // Check Wall Collisions
        g_collisions = players_check_wall_collisions(g_collisions, p_player);

        // Check board g_collisions
        g_collisions = players_check_board_collisions(g_collisions, player_team_color, p_player);

        // Apply queued board updates to board only, vram updates are deferred
        // until collision tests are over for the frame in: player_apply_queued_vram_updates()
        uint16_t idx;
        while (g_board_update_count_cur_player != g_board_update_count) {
            idx = board_update_queue[g_board_update_count_cur_player++];
            // if (idx > ((BOARD_BUF_W * BOARD_DISP_H) + BOARD_DISP_W)) {
            //     EMU_printf("Overflow Warning num=%d idx=%d > 596, x=%d (%d), y=%d (%d): NX x=%d (%d), y=%d (%d)\n",
            //         c, idx, 
            //         p_player->x.h, p_player->x.h / 8, p_player->y.h, p_player->y.h / 8,
            //         p_player->next_x.h, p_player->next_x.h / 8, p_player->next_y.h, p_player->next_y.h / 8);
            // }
            gameinfo.board[ idx ] = player_team_color;
        }

        // Handle bounce angle if there was a collision (and don't update to new next x,y)
        uint8_t angle = p_player->angle;

        if (g_collisions) {

            switch (g_collisions) {
                case COLLIDE_X:
                    angle = (uint8_t)(ANGLE_TO_8BIT(360) - angle);
                    break;

                case COLLIDE_Y:
                    angle = (uint8_t)(ANGLE_TO_8BIT(180) - angle);
                    break;

                case (COLLIDE_X | COLLIDE_Y):
                    angle = (uint8_t)(ANGLE_TO_8BIT(360) - angle);
                    angle = (uint8_t)(ANGLE_TO_8BIT(180) - angle);
                    break;
            }

            p_player->angle = angle = (uint8_t)(angle + (int8_t)(rand() & 0x03u) - 1);

            // // Recalc player  movement
            p_player->speed_x = (int16_t)SIN_PREMULT(angle);
            // Flip Y direction since adding positive values moves further down the screen
            p_player->speed_y = (int16_t)COS_PREMULT(angle) * -1;
        } else {
            // Otherwise update position to new location
            p_player->x.w = p_player->next_x.w;
            p_player->y.w = p_player->next_y.w;
        }
        p_player++;
    }
}


// Applies the board tile color changes to the screen (tilemap)
// which were queued during the frame
// Optimize: Using 2 arrays is probably slower than packing the 3 bytes into one array
void players_apply_queued_vram_updates(void) {
    for (uint8_t queued_idx = 0; queued_idx < g_board_update_count; queued_idx++) {
        uint16_t idx = board_update_queue[queued_idx];
        uint8_t player_team_color = gameinfo.board[idx];
        set_vram_byte(_SCRN0 + idx, player_team_color);
    }
}

/////////////////////////////////////////////////////////////////////////////

/*
// OPTIONAL: player distribution aproaches that didn't make the cut


// Initialize the players in a ring
void players_init_circle(void) {

    uint8_t * p_board = gameinfo.board;
    uint8_t angle_step = 256u / gameinfo.player_count;


    for (uint8_t c = 0u; c < gameinfo.player_count; c++) {
        gameinfo.players[c].x.h = (uint8_t)((SIN(c * angle_step) * ((SCREENWIDTH  / 4) + (rand() & 0x1Fu))) >> 7) + (SCREENWIDTH / 2);
        gameinfo.players[c].y.h = (uint8_t)((COS(c * angle_step) * ((SCREENHEIGHT / 4) + (rand() & 0x1Fu))) >> 7) + (SCREENHEIGHT / 2);
    }
}


// Initialize players into 4 team locations, but with random angle 
// so they radiate out from the center of each group on startup
void players_init_radiate_from_groups(void) {

    for(uint8_t c = 0; c < PLAYER_COUNT_MAX; c++) {
        // Looks better initially but less interesting thereafter due to pre-grouping
        // Probable solution: On first bounce switch to whatever color they touch or Random, if a wall then no change.
        // But that's not as easy now that team number is hard-wired as a mask of player index number
        
        // Match team locations for players 1-4, but with random angle
        gameinfo.players[c].x.w   = gameinfo.players[c & PLAYER_TEAMS_MASK].x.w;
        gameinfo.players[c].y.w   = gameinfo.players[c & PLAYER_TEAMS_MASK].y.w;
        gameinfo.players[c].angle = rand();
    }
}
*/


// Initialize the players as a grid always divided to match the number of players
// Optionally pairs with: board_init_grid_for_all_sizes()
static void players_init_grid(uint8_t player_count_idx) {

    uint8_t * p_board = gameinfo.board;
    uint8_t team;

    const uint8_t divs_x = players_divs_x[player_count_idx];
    const uint8_t divs_y = players_divs_y[player_count_idx];

    // Fixed point calc for widths to deal with cases where board width isn't evenly divisible
    const uint16_t step_x = (BOARD_DISP_W << 8) / (uint16_t)divs_x;
    const uint16_t step_y = (BOARD_DISP_H << 8) / (uint16_t)divs_y;

    // Divide the board into NxN regions, fill each region with a given team's color
    for (uint16_t y = 0; y < divs_y; y++) {
        for (uint16_t x = 0; x < divs_x; x++) {

            // Repeating 2x2 grid of 4 different colors
            team = (x & 0x01u) + ((y & 0x01u) * 2u);

            // Player assignment is on a repeating 2x2 meta grid, left->right, top->bottom
            // X offset for grid: (2 columns and 2 rows) of x for each row of 2x2 groups: (x/2u) * 4u
            // Y offset for grid: Max x offset per full rows of 2x2: (divs_x / 2) * 4u) multiplied by y / 2u for every 2 rows
            uint8_t player_idx = team + ((x / 2u) * 4u) +
                                               ((y / 2u) * ((divs_x / 2u) * 4u));
            gameinfo.players[player_idx].x.w   = (((x * step_x) + (step_x / 2)) * 8) - ((SPRITE_WIDTH / 2) << 8);
            gameinfo.players[player_idx].y.w   = (((y * step_y) + (step_y / 2)) * 8) - ((SPRITE_HEIGHT / 2) << 8);
            gameinfo.players[player_idx].angle = player_init_2x2_Angle[team];
        }
    }
}


#define EDGE_BORDER_SZ     (04u)
#define EDGE_BORDER_2x_SZ  (EDGE_BORDER_SZ * 2u)
#define NEAR_MASK (~0x0Fu) // Within 7 // 15 pixels

#define PLAYER_RANDOM_X() (((rand() % (PLAYER_RANGE_X_U8 - EDGE_BORDER_2x_SZ)) + PLAYER_MIN_X_U8 + EDGE_BORDER_SZ) << 8u)
#define PLAYER_RANDOM_Y() (((rand() % (PLAYER_RANGE_Y_U8 - EDGE_BORDER_2x_SZ)) + PLAYER_MIN_Y_U8 + EDGE_BORDER_SZ) << 8u)

#define DECLUMP_PASS_1  1u
#define DECLUMP_PASS_LIMIT  3u

static bool player_declump(uint8_t player_idx, uint8_t player_idx_max) { // , uint8_t pass_num) {

    bool relocated = false;

    for (uint8_t c = 0u; c < player_idx_max; c++) {

        if ( ((gameinfo.players[player_idx].x.h & NEAR_MASK) == (gameinfo.players[c].x.h & NEAR_MASK))
             && ((gameinfo.players[player_idx].y.h & NEAR_MASK) == (gameinfo.players[c].y.h & NEAR_MASK))) {

            gameinfo.players[player_idx].x.w   = PLAYER_RANDOM_X();
            gameinfo.players[player_idx].y.w   = PLAYER_RANDOM_Y();
            gameinfo.players[player_idx].angle = 0x7Fu - gameinfo.players[c].angle;
            relocated = true;
        }
    }
    return relocated;
}


// Init players to random locations and angles
// More interesting to watch over time, but less appealing at the start
// due to looking chatoic on random team bg colors
static void players_init_random(void) {

    for(uint8_t c = 0; c < PLAYER_COUNT_MAX; c++) {

        gameinfo.players[c].angle = (uint8_t)rand();
        gameinfo.players[c].x.w   = PLAYER_RANDOM_X();
        gameinfo.players[c].y.w   = PLAYER_RANDOM_Y();

        if (c > 0) {
            // Short two pass to try to prevent overlap of balls at random locations
            if (player_declump(c, c - 1u))
                player_declump(c, c - 1u);
        }
    }

}


void players_reset(void) {

    uint8_t player_count_idx = settings_get_setting_index(MENU_PLAYERS);

    // First always do an init with max num players so that they get initialized
    // and don't all show up at 0,0 if the user increases player count during gameplay
    players_init_random();

    // For alternate grid sizing, cap max grid sizing at 4 player setup
    if (!(KEY_PRESSED(BUTTON_SELECT_INIT_ALTERNATE))) {
        // Then initialize with the number that will actually be displayed on
        // starting if it's not the alternate mode
        players_init_grid(player_count_idx);
    }

    // Special case for the minimal player count to look better:
    // Override 2x1
    if (gameinfo.player_count == PLAYERS_2_VAL) {
        // Give players opposite angles
        gameinfo.players[0].angle = ANGLE_TO_8BIT(45u);
        gameinfo.players[1].angle = ANGLE_TO_8BIT(225u);
    }

    // Don't need to initialize player next x/y since 
    // that happens on update for each frame 

    players_all_recalc_movement();
}
