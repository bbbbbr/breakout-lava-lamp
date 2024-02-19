#include <gbdk/platform.h>
#include <string.h>
#include <stdint.h>
#include <gbdk/emu_debug.h>
#include <stdbool.h>
#include <rand.h>

#include "common.h"

#include "save_and_restore.h"
#include <cartsave.h>


// Quick checksum on all bytes of the stats record except the checksum byte itself
static uint8_t savedata_calc_checksum(void) {

    uint8_t * p_save_data = ((uint8_t *)&gameinfo) + sizeof(gameinfo.save_checksum);
    uint8_t checksum = 0x00u;

    // Calculate checksum for save, skip past checksum (first byte)
    for (uint16_t c = sizeof(gameinfo.save_checksum); c < (uint8_t) sizeof(gameinfo); c++) {
        checksum += *p_save_data++;
    }

    return checksum;
}


// Zeros out stat struture and if supported saves the change
void savedata_reset(void) {

    // Set all bytes to zero by default
    memset((void *)&gameinfo, 0x00u, sizeof(gameinfo));

    gameinfo.save_check0 = SAVEDATA_SIG_CHECK_0;
    gameinfo.save_check1 = SAVEDATA_SIG_CHECK_1;
    gameinfo.save_version = SAVE_VERSION_NUM;


    // Power-on defaults

    gameinfo.player_count = PLAYERS_4_VAL;
    gameinfo.player_count_last = gameinfo.player_count;
    gameinfo.speed        = SPEED_SLOW_VAL;
    gameinfo.action       = ACTION_CONTINUE_VAL;

    gameinfo.sprites_enabled = true;
    gameinfo.enable_sprites_next_frame = false;
    gameinfo.is_initialized = false;
    gameinfo.user_rand_seed.w = 0x0000u;
    gameinfo.system_rand_seed_saved = 0x1234u;

    // Update checksum: needs to happen after all other save data is initialized above
    gameinfo.save_checksum = savedata_calc_checksum();

    // For relevant carts, save the reset stats
    #if defined(CART_31k_1kflash) || defined(CART_mbc5)
        cartsave_save_data();
    #endif
}


// Initialize settings and stats on power-up, try to load values from cart sram/flash
void savedata_load(void) {

    #if defined(CART_31k_1kflash) || defined(CART_mbc5)
        // Check signature, reset save data and notify if signature failed
        // It is expected to fail on first power-up
        cartsave_restore_data();

        if ((gameinfo.save_check0 != SAVEDATA_SIG_CHECK_0) ||
            (gameinfo.save_check1 != SAVEDATA_SIG_CHECK_1) ||
            (gameinfo.save_checksum != savedata_calc_checksum() )) {
            savedata_reset();
        } else {
            // Validation passed, reload the system RNG state
            __rand_seed = gameinfo.system_rand_seed_saved;
        }
    #else
        // (bare 32k) Saveless cart just resets stats instead of loading
        savedata_reset();
    #endif
}


// Called after end of a game and before title/splash screen is displayed
void savedata_save(void) {

    // Add the system RNG state to the data being saved
    gameinfo.system_rand_seed_saved = __rand_seed;

    // Update checksum: needs to happen after all other save data is prepared above
    gameinfo.save_checksum = savedata_calc_checksum();

    // For relevant carts, save the reset stats
    #if defined(CART_31k_1kflash) || defined(CART_mbc5)
        cartsave_save_data();
    #endif
}


