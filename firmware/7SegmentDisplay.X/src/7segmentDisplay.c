
#include <xc.h>
#include "7segmentDisplay.h"


// *** macros ******************************************************************
//#define _XTAL_FREQ 20000000
#define _XTAL_FREQ 4000000


#define DIGIT_1  0x01
#define DIGIT_2  0x02
#define DIGIT_3  0x04
#define DIGIT_4  0x08
#define display_enableDigit(x)  PORTA = (x)
#define display_setDigitValue(x) PORTB = ((unsigned char)x)
#define display_setDigitOff() PORTB = 0xFF

#define ON_TIME_STEP  6
#define OFF_TIME      33

// *** internal typedefs *******************************************************

typedef enum slotTransitionMode_e
{
    SLOT_TRANS_MODE_INSTANT,
    SLOT_TRANS_MODE_SCROLL
} slotTransitionMode_t;

typedef enum segValue_e
{
    SEGVALUE_NONE = 0b11111111,
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

segmentDisplay_t display; // config
segValue_t currentDigits[4]; // currently displayed digits

state_t state;
unsigned char isr_onTimeCounter;
unsigned char isr_offTimeCounter;
unsigned char tickCounter;

segValue_t isr_currentDigits[4]; // in isr calculated digits


// *** function definitions ****************************************************

//void display_setBrightness(unsigned char percent)
//{
//    
//}

// brings pins into initial state

void display_init(void)
{
    ADCON1 = 0x06; // PortA as digital I/O

    display_enableDigit(DIGIT_1);
    display_setDigitOff();

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
    display.showColon = 1;
    display.brightness = 100;

    // 
    currentDigits[0] = display.slots[display.activeSlot].digits[0];
    currentDigits[1] = display.slots[display.activeSlot].digits[1];
    currentDigits[2] = display.slots[display.activeSlot].digits[2];
    currentDigits[3] = display.slots[display.activeSlot].digits[3];

    isr_currentDigits[0] = display.slots[display.activeSlot].digits[0];
    isr_currentDigits[1] = display.slots[display.activeSlot].digits[1];
    isr_currentDigits[2] = display.slots[display.activeSlot].digits[2];
    isr_currentDigits[3] = display.slots[display.activeSlot].digits[3];

    // minimal brightness
    onTimeCounter = isr_onTimeCounter = 0;
    offTimeCounter = isr_offTimeCounter = OFF_TIME;

    state = STATE_STARTUP;
    tickCounter = 0;
}

void display_handler(void)
{
    unsigned char i;

    // neue WErte aus ISR in lokale Arbeitsvariablen kopieren
    // (dabei Interrupt sperren)
    INTCONbits.GIE = 0;
    onTimeCounter = isr_onTimeCounter;
    offTimeCounter = isr_offTimeCounter;

    currentDigits[0] = isr_currentDigits[0];
    currentDigits[1] = isr_currentDigits[1];
    currentDigits[2] = isr_currentDigits[2];
    currentDigits[3] = isr_currentDigits[3];

    INTCONbits.GIE = 1;


    // digit 1
    display_enableDigit(DIGIT_1);
    display_setDigitValue(currentDigits[0]);

    for(i = 0; i < onTimeCounter; i++)
        __delay_us(2);
    display_setDigitOff();

    // digit 2
    display_enableDigit(DIGIT_2);
    display_setDigitValue(currentDigits[1]);

    for(i = 0; i < onTimeCounter; i++)
        __delay_us(2);
    display_setDigitOff();

    // digit 3
    display_enableDigit(DIGIT_3);
    display_setDigitValue(currentDigits[2]);

    for(i = 0; i < onTimeCounter; i++)
        __delay_us(2);
    display_setDigitOff();

    // digit 4
    display_enableDigit(DIGIT_4);
    display_setDigitValue(currentDigits[3]);
    for(i = 0; i < onTimeCounter; i++)
        __delay_us(2);
    display_setDigitOff();

    // off time
    for(i = 0; i < offTimeCounter; i++)
        __delay_us(500);
}

void display_cyclicTasks(void)
{
    tickCounter++;

    switch(state)
    {
    case STATE_STARTUP:
        if(tickCounter & 0x01)
        {
            isr_onTimeCounter += ON_TIME_STEP;
            isr_offTimeCounter--;

            if(isr_offTimeCounter == 0)
            {
                tickCounter = 0;
                state = STATE_DISPLAY;
            }
        }
        break;
    case STATE_DISPLAY:
        if(display.showColon)
        {
            isr_currentDigits[1] &= 0x7F;
        }
        
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
                isr_currentDigits[3] = SEGVALUE_NONE;

                state = STATE_TRANSIT_BLANK;
            }
            else
            {
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
                
            tickCounter = 0;
            state = STATE_DISPLAY;
        }
        break;
    }
}

