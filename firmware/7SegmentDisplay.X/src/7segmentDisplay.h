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
    SEGVALUE_1 = 0b11111001,
    SEGVALUE_2 = 0b10100100,
    SEGVALUE_3 = 0b10110000,
    SEGVALUE_4 = 0b10011001,
    SEGVALUE_5 = 0b10010010,
    SEGVALUE_6 = 0b10000010,
    SEGVALUE_7 = 0b11111000,
    SEGVALUE_8 = 0b10000000,
    SEGVALUE_9 = 0b10010000,
    SEGVALUE_U = 0b11000001,
    SEGVALUE_MINUS = 0b10111111
} segValue_t;

typedef struct displayValue_s
{
    unsigned char enabled;
    segValue_t digits[4]; // output pin config
} displayValue_t;

typedef struct displayConfig_s
{
    unsigned short slotDuration;
    unsigned short scrollDuration;
    slotTransitionMode_t transitionMode;
    unsigned char scrollBlanksCount;
    unsigned char showColon;
    unsigned char brightnessInPercent;
} displayConfig_t;

// *** functions ***************************************************************
void display_init(void);
void display_generatePwm(void);
void display_calcNextValueToShow(void);
void display_recalcPwmDutyCycle(void);
void display_setConfig(displayConfig_t* config);
void display_setConfig_brightness(unsigned char percent);
void display_setSlotDisplayValue(unsigned char slotNumber, displayValue_t* value);

#endif	/* SEVENSEGMENTDISPLAY_H */

