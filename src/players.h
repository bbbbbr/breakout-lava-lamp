#ifndef __PLAYERS_H
#define __PLAYERS_H

void players_redraw_sprites(void);
void player_recalc_movement(uint8_t idx);
void players_all_recalc_movement(void);
bool player_board_check_xy(uint8_t x, uint8_t y, uint8_t board_player_col);
void player_check_board_collisions(uint8_t player_id);
void player_check_wall_collisions(uint8_t player_id);
void players_update(void);
void players_reset(void);

#endif // __PLAYERS_H


