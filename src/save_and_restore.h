#ifndef _SAVE_AND_RESTORE_H
#define _SAVE_AND_RESTORE_H

#include <gbdk/bcd.h>

#define SAVE_FRAMES_THRESHOLD  60u  // Save about once per second (if using EEPROM carts make this configurable based on the cart)

void savedata_load(void);
void savedata_save(void);
void savedata_reset(void);



#endif // _SAVE_AND_RESTORE_H


