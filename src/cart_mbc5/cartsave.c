#include <gbdk/platform.h>
#include <stdint.h>
#include <string.h>

#include "../common.h"


// Map a secondary stats struct to the beginning of SRAM (0xA000) when using MBC5
gameinfo_t __at (0xA000) sram_savedata;


void cartsave_restore_data(void) {

    ENABLE_RAM_MBC5;
    memcpy((void *)&gameinfo, (void *)&sram_savedata, sizeof(gameinfo));
    DISABLE_RAM_MBC5;
}


// TODO: warning on failure to save?
void cartsave_save_data(void) {

    ENABLE_RAM_MBC5;
    memcpy((void *)&sram_savedata, (void *)&gameinfo, sizeof(gameinfo));
    DISABLE_RAM_MBC5;
}



