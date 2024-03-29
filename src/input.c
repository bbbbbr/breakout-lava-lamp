#include <gbdk/platform.h>
#include <stdint.h>

#include "input.h"

uint8_t keys = 0x00;
uint8_t previous_keys = 0x00;
uint8_t key_repeat_count = 0x00;

// Reduce CPU usage by only checking once per frame
// Allows a loop control to be passed in
void waitpadticked_lowcpu(uint8_t button_mask) {

    while (1) {

        vsync(); // yield CPU
        UPDATE_KEYS();
        if (KEY_TICKED(button_mask))
            break;
    };

    // Prevent passing through any key ticked
    // event that may have just happened
    UPDATE_KEYS();
}
