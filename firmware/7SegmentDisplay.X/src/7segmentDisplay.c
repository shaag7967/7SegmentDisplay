
#include <xc.h>
#include "7segmentDisplay.h"


// *** macros ******************************************************************
//#define _XTAL_FREQ 20000000
#define _XTAL_FREQ 4000000

#define LED_RESOLUTION   2  // us

#define DIGIT_1  0x01
#define DIGIT_2  0x02
#define DIGIT_3  0x04
#define DIGIT_4  0x08
#define display_enableDigit(x)  PORTA = (x)
#define display_disableDigit()  PORTA = 0x00
#define display_setDigitValue(x) PORTB = ((unsigned char)x)


// *** internal typedefs *******************************************************

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

typedef struct displaySlot_s
{
    segValue_t digits[4]; // output pin levels
    unsigned char enabled;
} displaySlot_t;

typedef struct segmentDisplay_s
{
    displaySlot_t slots[3];
    unsigned short slotDuration;
    unsigned short scrollDuration;
    unsigned char activeSlot;
    slotTransitionMode_t transitionMode;
    unsigned char showColon;
    unsigned char brightness;
} segmentDisplay_t;

typedef enum state_e
{
    STATE_STARTUP,
    STATE_DISPLAY,
    STATE_TRANSIT_BLANK,
    STATE_TRANSIT_DIG_1,
    STATE_TRANSIT_DIG_2,
    STATE_TRANSIT_DIG_3,
    STATE_TRANSIT_DIG_4
} state_t;

// *** internal variables ******************************************************
unsigned char onTimeCounter;
unsigned char offTimeCounter;
unsigned char isr_onTimeCounter;
unsigned char isr_offTimeCounter;

segmentDisplay_t display; // config
segValue_t currentDigits[4]; // currently displayed digits
segValue_t isr_currentDigits[4]; // in isr calculated digits

state_t state;

unsigned char tickCounter;
unsigned char currentBrightness;

// *** led values **************************************************************
//const unsigned char ledPwmValues[] = {0,1,1,2,2,2,2,2,3,3,3,4,4,4,5,5,6,7,7,8,
//   9,10,11,13,14,16,17,19,22,24,27,30,33,37,41,45,50,56,62,69,77,86,95,106,118,
//   131,146,162,180,200};

//const unsigned char ledPwmValues[] = {0,4,4,5,6,7,9,10,12,14,17,21,24,29,35,41,49,59,70,83,99,118,141,168,200};


const unsigned char ledPwmValues[100] = {0,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,
    3,3,4,4,4,4,4,5,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,11,12,13,13,14,15,16,17,
    17,18,19,20,22,23,24,25,27,28,30,31,33,35,37,39,41,43,45,48,50,53,56,59,62,
    66,69,73,77,81,86,90,95,100,106,112,118,124,131,138,146,153,162,171,180,190,200};


#define MAX_BRIGHTNESS_PWM_VALUE   (ledPwmValues[sizeof(ledPwmValues)/sizeof(ledPwmValues[0]) - 1])

// *** function definitions ****************************************************

void display_init(void)
{
    ADCON1 = 0x06; // PortA as digital I/O

    display_enableDigit(DIGIT_1);
    display_setDigitValue(SEGVALUE_OFF);

    TRISA = 0xF0;
    TRISB = 0x00;

    display.slots[0].digits[0] = SEGVALUE_H;
    display.slots[0].digits[1] = SEGVALUE_A;
    display.slots[0].digits[2] = SEGVALUE_A;
    display.slots[0].digits[3] = SEGVALUE_G;
    display.slots[0].enabled = 0;

    display.slots[1].digits[0] = SEGVALUE_ONE;
    display.slots[1].digits[1] = SEGVALUE_TWO;
    display.slots[1].digits[2] = SEGVALUE_THREE;
    display.slots[1].digits[3] = SEGVALUE_FOUR;
    display.slots[1].enabled = 0;

    display.slots[2].digits[0] = SEGVALUE_MINUS;
    display.slots[2].digits[1] = SEGVALUE_MINUS;
    display.slots[2].digits[2] = SEGVALUE_MINUS;
    display.slots[2].digits[3] = SEGVALUE_MINUS;
    display.slots[2].enabled = 0;

    display.slotDuration = 150;
    display.scrollDuration = 15;
    display.activeSlot = 0;
    display.transitionMode = SLOT_TRANS_MODE_SCROLL;
//    display.transitionMode = SLOT_TRANS_MODE_INSTANT;
    display.showColon = 1;
    display.brightness = 99;

    // 
    currentDigits[0] = display.slots[display.activeSlot].digits[0];
    currentDigits[1] = display.slots[display.activeSlot].digits[1];
    currentDigits[2] = display.slots[display.activeSlot].digits[2];
    currentDigits[3] = display.slots[display.activeSlot].digits[3];

    isr_currentDigits[0] = display.slots[display.activeSlot].digits[0];
    isr_currentDigits[1] = display.slots[display.activeSlot].digits[1];
    isr_currentDigits[2] = display.slots[display.activeSlot].digits[2];
    isr_currentDigits[3] = display.slots[display.activeSlot].digits[3];

    // minimale Helligkeit
    currentBrightness = 0;
    isr_onTimeCounter = ledPwmValues[currentBrightness];
    isr_offTimeCounter = ledPwmValues[99] - isr_onTimeCounter;
    
    state = STATE_STARTUP;
    tickCounter = 0;
}

void display_handler(void)
{
    unsigned short i;

    onTimeCounter = isr_onTimeCounter;
    offTimeCounter = isr_offTimeCounter;

    currentDigits[0] = isr_currentDigits[0];
    currentDigits[1] = isr_currentDigits[1];
    currentDigits[2] = isr_currentDigits[2];
    currentDigits[3] = isr_currentDigits[3];


    // digit 1
    display_setDigitValue(currentDigits[0]);
    display_enableDigit(DIGIT_1);

    for(i = 0; i < onTimeCounter; i++)
        __delay_us(LED_RESOLUTION);
    
    display_disableDigit();
    
    for(i = 0; i < offTimeCounter; i++)
        __delay_us(LED_RESOLUTION);
    
    // digit 2
    display_setDigitValue(currentDigits[1]);
    display_enableDigit(DIGIT_2);

    for(i = 0; i < onTimeCounter; i++)
        __delay_us(LED_RESOLUTION);
    
    display_disableDigit();
    
    for(i = 0; i < offTimeCounter; i++)
        __delay_us(LED_RESOLUTION);
    
    // digit 3
    display_setDigitValue(currentDigits[2]);
    display_enableDigit(DIGIT_3);
    
    for(i = 0; i < onTimeCounter; i++)
        __delay_us(LED_RESOLUTION);
    
    display_disableDigit();
    
    for(i = 0; i < offTimeCounter; i++)
        __delay_us(LED_RESOLUTION);
    
    // digit 4
    display_setDigitValue(currentDigits[3]);
    display_enableDigit(DIGIT_4);
    
    for(i = 0; i < onTimeCounter; i++)
        __delay_us(LED_RESOLUTION);
    
    display_disableDigit();
    
    for(i = 0; i < offTimeCounter; i++)
        __delay_us(LED_RESOLUTION);
}

void display_cyclicTasks(void)
{
    tickCounter++;

    switch(state)
    {
    case STATE_STARTUP:
        if(tickCounter & 0x10)
        {
            isr_onTimeCounter = ledPwmValues[currentBrightness];
            isr_offTimeCounter = ledPwmValues[99] - isr_onTimeCounter;

            currentBrightness++;

            if(currentBrightness >= display.brightness)
            {
                if(display.showColon)
                {
                    isr_currentDigits[1] &= 0x7F;
                }

                tickCounter = 0;
                state = STATE_DISPLAY;
            }
        }
        break;
    case STATE_DISPLAY:
        if(tickCounter == display.slotDuration)
        {
            tickCounter = 0;

            display.activeSlot++;
            if(display.activeSlot > 2)
                display.activeSlot = 0;

            if(display.transitionMode == SLOT_TRANS_MODE_SCROLL)
            {
                isr_currentDigits[0] = (isr_currentDigits[1] | 0x80);
                isr_currentDigits[1] = isr_currentDigits[2];
                isr_currentDigits[2] = isr_currentDigits[3];
                isr_currentDigits[3] = SEGVALUE_OFF;

                state = STATE_TRANSIT_BLANK;
            }
            else
            {
                if(display.showColon)
                {
                    isr_currentDigits[1] &= 0x7F;
                }

                isr_currentDigits[0] = display.slots[display.activeSlot].digits[0];
                isr_currentDigits[1] = display.slots[display.activeSlot].digits[1];
                isr_currentDigits[2] = display.slots[display.activeSlot].digits[2];
                isr_currentDigits[3] = display.slots[display.activeSlot].digits[3];
            }
        }
        break;
    case STATE_TRANSIT_BLANK:
        if(tickCounter == display.scrollDuration)
        {
            isr_currentDigits[0] = isr_currentDigits[1];
            isr_currentDigits[1] = isr_currentDigits[2];
            isr_currentDigits[2] = isr_currentDigits[3];
            isr_currentDigits[3] = display.slots[display.activeSlot].digits[0];
                
            tickCounter = 0;
            state = STATE_TRANSIT_DIG_1;
        }
        break;
    case STATE_TRANSIT_DIG_1:
        if(tickCounter == display.scrollDuration)
        {
            isr_currentDigits[0] = isr_currentDigits[1];
            isr_currentDigits[1] = isr_currentDigits[2];
            isr_currentDigits[2] = isr_currentDigits[3];
            isr_currentDigits[3] = display.slots[display.activeSlot].digits[1];
                
            tickCounter = 0;
            state = STATE_TRANSIT_DIG_2;
        }
        break;
    case STATE_TRANSIT_DIG_2:
        if(tickCounter == display.scrollDuration)
        {
            isr_currentDigits[0] = isr_currentDigits[1];
            isr_currentDigits[1] = isr_currentDigits[2];
            isr_currentDigits[2] = isr_currentDigits[3];
            isr_currentDigits[3] = display.slots[display.activeSlot].digits[2];
                
            tickCounter = 0;
            state = STATE_TRANSIT_DIG_3;
        }
        break;
    case STATE_TRANSIT_DIG_3:
        if(tickCounter == display.scrollDuration)
        {
            isr_currentDigits[0] = isr_currentDigits[1];
            isr_currentDigits[1] = isr_currentDigits[2];
            isr_currentDigits[2] = isr_currentDigits[3];
            isr_currentDigits[3] = display.slots[display.activeSlot].digits[3];
            
            if(display.showColon)
            {
                isr_currentDigits[1] &= 0x7F;
            }
            
            tickCounter = 0;
            state = STATE_DISPLAY;
        }
        break;
    }
}

