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


// offset to center the 8X8 circle sprite on the x,y coordinate
#define SPR_OFFSET_X (DEVICE_SPRITE_PX_OFFSET_X)
#define SPR_OFFSET_Y (DEVICE_SPRITE_PX_OFFSET_Y)

// Player corner calculations (Sprite is 8 pixels wide, Upper Left is 0,0, Lower Right is +7,+7)
#define PLAYER_LEFT(px)   ((px                     ) / BOARD_GRID_SZ)
#define PLAYER_RIGHT(px)  ((px + (SPRITE_WIDTH - 1)) / BOARD_GRID_SZ)
#define PLAYER_TOP(py)    ((py                     ) / BOARD_GRID_SZ)
#define PLAYER_BOTTOM(py) ((py + (SPRITE_HEIGHT - 1)) / BOARD_GRID_SZ)


extern uint16_t board_update_queue[PLAYER_TEAMS_COUNT];
extern uint8_t board_update_count;


// TODO: Maybe inline this
void players_redraw_sprites_asm(void) NAKED {
      __asm \

    _GINFO_OFSET_PLAYER_COUNT = 6  // ; location of (uint8_t) .player_count
    _GINFO_OFSET_PLAYERS      = 17 // ; start of (player_t) .players[N]

    _PLAYER_TYPE_Y_HI_OFFSET      = 2

    _PLAYER_TYPE_X_HI_OFFSET      = 9


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




uint16_t g_player_cur_x;
uint16_t g_player_cur_y;
uint16_t g_player_next_x;
uint16_t g_player_next_y;
uint8_t  g_player_speed_neg_x;
uint8_t  g_player_speed_neg_y;
uint8_t g_player_idx;

void players_update_asm(void) NAKED {
      __asm \

    _PLAYER_MAX_X_U8 = (DEVICE_SCREEN_WIDTH * 8) - _SPRITE_WIDTH
    _PLAYER_MAX_Y_U8 = (DEVICE_SCREEN_HEIGHT * 8) - _SPRITE_HEIGHT
    _GINFO_OFSET_PLAYER_COUNT = 6  // ; location of (uint8_t) .player_count
    _GINFO_OFSET_PLAYERS      = 17 // ; start of (player_t) .players[N]

    // ; Offsets into Player Struct
    _PLAYER_TYPE_Y_ANGLE          = 0

    _PLAYER_TYPE_Y_OFFSET         = 1
    _PLAYER_TYPE_Y_HI_OFFSET      = 2
    _PLAYER_TYPE_NEXT_Y_OFFSET    = 5
    _PLAYER_TYPE_NEXT_Y_HI_OFFSET = 6

    _PLAYER_TYPE_X_OFFSET         = 8
    _PLAYER_TYPE_X_HI_OFFSET      = 9
    _PLAYER_TYPE_NEXT_X_OFFSET    = 12
    _PLAYER_TYPE_NEXT_X_HI_OFFSET = 13

    push af // 4
    push hl // 4
    push bc // 4
    push de // 4

._players_main_loop_init:
    // == Set up Counter for players ==
    // ; players_count = gameinfo.player_count;
    ld  a, (#_gameinfo + #_GINFO_OFSET_PLAYER_COUNT) // ; Counter max: gameinfo.player_count
    ld  (#_g_player_idx), a


    // Load Array of player_t structs
    ld  hl, #_gameinfo + #(_GINFO_OFSET_PLAYERS  + _PLAYER_TYPE_Y_OFFSET) // ; &gameinfo.players[0].y.l

._players_main_loop:  // ; main player increment loop

    // == Cache current Y postion and player pointer ==
    push hl  // ; points to .y.l
    inc hl
    ld  a, (hl+) // ; save .y.h -> g_player_y
    ld  (#_g_player_cur_y + 1), a


    // == Calc next Y position + Wall Bounce test ==
    // ; HL points to .speed_y
    // ; p_player->next_y.w += p_player->speed_y;
    ld  a, (hl+)  // ; load .speed_y.l
    ld  c, a
    ld  a, (hl+)  // ; load .speed_y.h
    ld  b, a

        // Check and save Y direction via sign bit for int16_t
        and a, #0x80
        ld  (#_g_player_speed_neg_y), a

        // ; .next_y.w(BC) += .speed_y;
        ld  a, c
        add (hl)
        ld  (hl+), a
        ld  (#_g_player_next_y), a // ; Cache next_y.l

        ld  a, b
        adc (hl)
        ld  (hl+), a
        ld  (#_g_player_next_y + #1), a // ; Cache next_y.h
        // A has next_y.h

        // ; == Wall bounce test for Y axis ==
        // ; if (p_player->next_y.h > PLAYER_MAX_Y_U8)
        // ; HL points to .bounce_y
        // ; A has next_y.h
        cp  a, #(_PLAYER_MAX_Y_U8 - 1) // ; -1 to make it > instead of >=
        ccf           // ; invert cary flag (NC if A >= N8) -> (NC if A < N8)
        ld  a, #0     // ; clear A without resetting carry flag
        rla           // ; Load result of carry flag to A
        ld  (hl+), a  // ; save to  .bounce_y

    // == Cache current X postion ==
    // ; HL points to .x
    inc hl
    ld  a, (hl+) // ; save .x.h -> g_player_x
    ld  (#_g_player_cur_x + 1), a

    // == Calc next X position + Wall Bounce test ==
    // ; HL points to .speed_x
    // ; p_player->next_x.w += p_player->speed_x;
    ld  a, (hl+) // ; load .speed_x
    ld  c, a
    ld  a, (hl+)
    ld  b, a

        // Check and save Y direction via sign bit for int16_t
        and a, #0x80
        ld  (#_g_player_speed_neg_x), a

        // ; .next_x.w(BC) += .speed_x;
        ld  a, c
        add (hl)
        ld  (hl+), a
        ld  (#_g_player_next_x), a // ; Cache next_x.l

        ld  a, b
        adc (hl)
        ld  (hl+), a
        ld  (#_g_player_next_x + #1), a // ; Cache next_x.h
        // A has next_x.h


        // ; == Wall bounce test for X axis ==
        // ; if (p_player->next_x.h > PLAYER_MAX_X_U8)
        // ; HL points to .bounce_x
        // ; A has next_x.h
        cp  a, #(_PLAYER_MAX_X_U8 - 1) // ; -1 to make it > instead of >=
        ld  a, #0     // ; clear A without resetting carry flag
        ccf           // ; invert cary flag (NC if A >= N8) -> (NC if A < N8)
        rla           // ; Load result of carry flag to A
        ld  (hl+), a // ; save to  .bounce_x




    // ; == Check Board Collision ==

        // // Check Horizontal movement (New X & Current Y in order to avoid false triggers on Y)
        // // Test separately here since collision check also updates board tile color
        // if (p_player->speed_x != 0) {
        //     uint8_t test_x = (p_player->speed_x > 0) ? PLAYER_RIGHT(nx) : PLAYER_LEFT(nx);
        //
        //     if (player_board_check_xy(test_x, PLAYER_TOP(py),    player_team_color)) p_player->bounce_x = true;
        //     if (player_board_check_xy(test_x, PLAYER_BOTTOM(py), player_team_color)) p_player->bounce_x = true;
        // }

/*
        // == Check Horizontal movement ==
        // (Next X & Current Y in order to avoid false triggers on Y)
        // Test separately here since collision check also updates board tile color
        ld  a, #(g_player_speed_neg_y)
        or  a
        jr  NZ ._player_test_board_collide_right

        ._player_test_board_collide_left:
            // ; Test Top-Left:
            // ; Test Bottom-Left
            jr

        ._player_test_board_collide_right:

        ._player_test_board_horiz_done


*/


    // TODO
        // ; Revert position update if bounce happened
        // ld  a, (#_g_player_cur_x + 1)
        // ld  a, (#_g_player_cur_y + 1)



    // ; TODO: optimize



    // ; == Store updated X, Y values : TODO: if no bounce detected
    pop hl  // ; Restore pointer for player.y.l

    ld  a, (#_g_player_next_y)
    ld  (hl+), a
    ld  a, (#_g_player_next_y + #1)
    ld  (hl+), a

    inc hl  // ; Advance to player.x.l
    inc hl
    inc hl
    inc hl
    inc hl

    ld  a, (#_g_player_next_x)
    ld  (hl+), a
    ld  a, (#_g_player_next_x + #1)
    ld  (hl+), a

    // ; TODO: optimize increment
    inc hl  // ; Advance to start of Next player struct in array
    inc hl
    inc hl
    inc hl
    inc hl

    inc hl  // ; Advance to Next player.y.l

    // ; == Update loop counter ==
    ld  a, (#_g_player_idx)
    dec a
    ld  (#_g_player_idx), a
    jr  NZ, ._players_main_loop

    pop de // 3
    pop bc // 3
    pop hl // 3
    pop af // 3

    ret
   __endasm;
}



// This isn't much faster than the C version
// Most of the wait time is stalling for the right vram mode
void players_apply_queued_vram_updates_asm(void) {
    __asm \


    ld  bc, #_board_update_queue   // ; board_update_queue[]
    ld  a, (#_g_board_update_count)
    inc a
    // ; DE is scratch / pass u16 param

    .queue_loop:

        // Check for loop exit
        dec  a
        ret  Z
        push af

        // ; Load next address/index from queue into DE
        ld   a, (bc)
        ld   e, a
        inc  bc

        ld   a, (bc)
        ld   d, a
        inc  bc

        // ; Use index to retrieve Team Color from board
        ld  hl, #(_gameinfo + 497)     // ; gameinfo.board[]
        add  hl, de
        ld   a, (hl)

        // Add address/index to SCRN0 (map) in DE        
        // ; Lower address of map / SCRN0 is 0x00, so just need to add upper byte
        ld   l, a  // ; Save A
        ld   a, #>(__SCRN0)
        add  a, d
        ld   d, a
        ld   a, l  // ; restore A

        // ; team color in A, vram address in DE
        call    _set_vram_byte

        // ; Reload board array counter
        pop af

        jr  .queue_loop
    __endasm;
}
