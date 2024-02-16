#ifndef __PLAYERS_ASM_H
#define __PLAYERS_ASM_H

void players_update_asm(void) NAKED;
void players_redraw_sprites_asm(void) NAKED;
void players_apply_queued_vram_updates_asm(void);

#endif // __PLAYERS_ASM_H


