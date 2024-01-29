#ifndef _MATH_UTIL_H
#define _MATH_UTIL_H

extern const int8_t sine_table[];

// Range 0 - 255
#define SIN(angle)  ( sine_table[(uint8_t)angle] )
#define COS(angle)  ( sine_table[(uint8_t)((uint8_t)angle + (256u / 4u))] ) // relies on unsigned byte wraparound (0 - 255)

#define ANGLE_TO_8BIT(angle) (uint8_t)((angle / 360.0) * 255)

// #define ANGLE_0    ANGLE_TO_8BIT(  0u)
// #define ANGLE_45   ANGLE_TO_8BIT( 45u)
// #define ANGLE_90   ANGLE_TO_8BIT( 45u)
// #define ANGLE_135  ANGLE_TO_8BIT(135u)
// #define ANGLE_180  ANGLE_TO_8BIT(180u)
// #define ANGLE_225  ANGLE_TO_8BIT(225u)
// #define ANGLE_270  ANGLE_TO_8BIT(270u)
// #define ANGLE_315  ANGLE_TO_8BIT(315u)

// #define ANGLE_SPEED_TO_DELTA_X(angle, speed) (SIN(angle) * speed)
// #define ANGLE_SPEED_TO_DELTA_Y(angle, speed) (COS(angle) * speed)


#endif // _MATH_UTIL_H


