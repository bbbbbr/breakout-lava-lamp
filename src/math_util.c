#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
#include <stdint.h>

#include "common.h"
#include "math_util.h"

// Precalculated Angle * Speed lookup tables
int16_t sine_table_slowest[256];
int16_t sine_table_slow[256];
int16_t sine_table_fast[256];
int16_t sine_table_fastest[256];

const int16_t * p_sine_table;

const int8_t sine_table[256] = {
    0,    3,    6,    9,    12,   15,   18,   21,   24,   28,   31,   34,
    37,   40,   43,   46,   48,   51,   54,   57,   60,   63,   65,   68,
    71,   73,   76,   78,   81,   83,   85,   88,   90,   92,   94,   96,
    98,   100,  102,  104,  106,  108,  109,  111,  112,  114,  115,  117,
    118,  119,  120,  121,  122,  123,  124,  124,  125,  126,  126,  127,
    127,  127,  127,  127,  127,  127,  127,  127,  127,  127,  126,  126,
    125,  124,  124,  123,  122,  121,  120,  119,  118,  117,  115,  114,
    112,  111,  109,  108,  106,  104,  102,  100,  98,   96,   94,   92,
    90,   88,   85,   83,   81,   78,   76,   73,   71,   68,   65,   63,
    60,   57,   54,   51,   48,   46,   43,   40,   37,   34,   31,   28,
    24,   21,   18,   15,   12,   9,    6,    3,    0,    -3,   -6,   -9,
    -12,  -15,  -18,  -21,  -24,  -28,  -31,  -34,  -37,  -40,  -43,  -46,
    -48,  -51,  -54,  -57,  -60,  -63,  -65,  -68,  -71,  -73,  -76,  -78,
    -81,  -83,  -85,  -88,  -90,  -92,  -94,  -96,  -98,  -100, -102, -104,
    -106, -108, -109, -111, -112, -114, -115, -117, -118, -119, -120, -121,
    -122, -123, -124, -124, -125, -126, -126, -127, -127, -127, -127, -127,
    -127, -127, -127, -127, -127, -127, -126, -126, -125, -124, -124, -123,
    -122, -121, -120, -119, -118, -117, -115, -114, -112, -111, -109, -108,
    -106, -104, -102, -100, -98,  -96,  -94,  -92,  -90,  -88,  -85,  -83,
    -81,  -78,  -76,  -73,  -71,  -68,  -65,  -63,  -60,  -57,  -54,  -51,
    -48,  -46,  -43,  -40,  -37,  -34,  -31,  -28,  -24,  -21,  -18,  -15,
    -12,  -9,   -6,   -3,
};


void sine_tables_init(void) {
    uint8_t c;

    c = 0u;
    do {
        sine_table_slowest[c] = sine_table[c] * SPEED_SLOWEST_VAL;
        c++;
    } while (c != 0u);

    c = 0u;
    do {
        sine_table_slow[c] = sine_table[c] * SPEED_SLOW_VAL;
        c++;
    } while (c != 0u);

    c = 0u;
    do {
        sine_table_fast[c] = sine_table[c] * SPEED_FAST_VAL;
        c++;
    } while (c != 0u);

    c = 0u;
    do {
        sine_table_fastest[c] = sine_table[c] * SPEED_FASTEST_VAL;
        c++;
    } while (c != 0u);
}

void sine_table_select(uint8_t speed_value) {
    switch (speed_value) {

        case SPEED_SLOWEST_VAL: p_sine_table = sine_table_slowest;
            break;

        case SPEED_SLOW_VAL: p_sine_table = sine_table_slow;
            break;

        case SPEED_FAST_VAL: p_sine_table = sine_table_fast;
            break;

        case SPEED_FASTEST_VAL: p_sine_table = sine_table_fastest;
            break;
    }
}

