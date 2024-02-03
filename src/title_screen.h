#ifndef _TITLE_SCREEN_H
#define _TITLE_SCREEN_H

// Menu items
#define MENU_ACTION  0u
#define MENU_SPEED   1u
#define MENU_PLAYERS 2u
#define MENU_DEFAULT (MENU_ACTION)
#define MENU_MAX     (MENU_PLAYERS)

#define SPR_ID_ACTION  0u
#define SPR_ID_SPEED   1u
#define SPR_ID_PLAYERS 2u

#define SPR_CURSOR_ACTIVE     0u
#define SPR_CURSOR_NOT_ACTIVE 1u

// Defaults per menu
#define ACTION_DEFAULT  ACTION_CONTINUE
#define SPEED_DEFAULT   SPEED_SLOW
#define PLAYERS_DEFAULT PLAYERS_4



void title_init(void);
void title_run(void);

#endif // _TITLE_SCREEN_H


