/* 
 * File:   7segmentDisplay.h
 * Author: seven
 *
 * Created on December 2, 2017, 4:14 PM
 */

#ifndef SEVENSEGMENTDISPLAY_H
#define	SEVENSEGMENTDISPLAY_H


// *** typedefs ****************************************************************
typedef enum slotTransitionMode_e
{
    SLOT_TRANS_MODE_INSTANT,
    SLOT_TRANS_MODE_SCROLL
} slotTransitionMode_t;

typedef enum segValue_e
{
    SEGVALUE_OFF = 0b11111111,
    SEGVALUE_ONE = 0b11111001,
    SEGVALUE_TWO = 0b10100100,
    SEGVALUE_THREE = 0b10110000,
    SEGVALUE_FOUR = 0b10011001,
    SEGVALUE_NINE = 0b10010000,
    SEGVALUE_U = 0b11000001,
    SEGVALUE_H = 0b10001001,
    SEGVALUE_A = 0b10001000,
    SEGVALUE_G = 0b10000010,
    SEGVALUE_MINUS = 0b10111111
} segValue_t;

typedef struct displayValue_s
{
    segValue_t digits[4]; // output pin config
    unsigned char enabled;
} displayValue_t;

typedef struct displayConfig_s
{
    unsigned short slotDuration;
    unsigned short scrollDuration;
    slotTransitionMode_t transitionMode;
    unsigned char showColon;
    unsigned char brightness;
} displayConfig_t;

//    display.slotDuration = 150;
//    display.scrollDuration = 15;
//    display.transitionMode = SLOT_TRANS_MODE_SCROLL;
//    display.showColon = 1;
//    display.brightness = 90;


// *** functions ***************************************************************
void display_init(void);
void display_handler(void);
void display_cyclicTasks(void);

// *** display config **********************************************************
extern displayConfig_t display;
extern displayValue_t slots[3];

#endif	/* SEVENSEGMENTDISPLAY_H */

