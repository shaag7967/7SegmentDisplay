
#include <xc.h>
#include "7segmentDisplay.h"
#include <string.h>


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

#define NUMBER_OF_SLOTS   5

// *** internal typedefs *******************************************************

typedef enum state_e
{
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

unsigned char activeSlot;
displayConfig_t displayCfg;
displayValue_t slots[NUMBER_OF_SLOTS];
unsigned char blankDigitTransitionCounter; // number of empty digits when transitioning

segValue_t currentDigits[4]; // currently displayed digits

state_t state;

unsigned short tickCounter;

// *** led values **************************************************************
const unsigned char ledPwmValues[101] = {0,0,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,
    3,3,4,4,4,4,4,5,5,5,5,6,6,6,7,7,7,8,8,9,9,10,10,11,11,12,13,13,14,15,16,17,
    17,18,19,20,22,23,24,25,27,28,30,31,33,35,37,39,41,43,45,48,50,53,56,59,62,
    66,69,73,77,81,86,90,95,100,106,112,118,124,131,138,146,153,162,171,180,190,200};


// *** function definitions ****************************************************

void display_init(void)
{
    ADCON1 = 0x06; // PortA as digital I/O

    display_disableDigit();
    display_setDigitValue(SEGVALUE_OFF);

    TRISA = 0xF0;
    TRISB = 0x00;

    memset(slots, 0x00, sizeof(slots));

    displayCfg.slotDuration = 120;
    displayCfg.scrollDuration = 7;
    displayCfg.transitionMode = SLOT_TRANS_MODE_SCROLL;
    displayCfg.scrollBlanksCount = 2;
    displayCfg.showColon = 1;
    displayCfg.brightnessInPercent = 100;

    // 
    currentDigits[0] = SEGVALUE_8;
    currentDigits[1] = SEGVALUE_8;
    currentDigits[2] = SEGVALUE_8;
    currentDigits[3] = SEGVALUE_8;

    display_recalcPwmDutyCycle();
    
    state = STATE_DISPLAY;
    activeSlot = 0xFF;
    tickCounter = 0xFFFF;
}

void display_generatePwm(void)
{
    unsigned short i;

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

void display_calcNextValueToShow(void)
{
    switch(state)
    {
    case STATE_DISPLAY:
        if(tickCounter >= displayCfg.slotDuration)
        {
            unsigned char currentSlot = activeSlot;
            unsigned char slotCount = sizeof(slots) / sizeof(displayValue_t);
            
            tickCounter = 0;

            // find next enabled slot
            do
            {
                activeSlot++;
                slotCount--;
                if(activeSlot > (NUMBER_OF_SLOTS - 1))
                    activeSlot = 0;
            } while(slots[activeSlot].enabled == 0 && slotCount > 0);
            
            if(currentSlot == activeSlot || slotCount == 0)
                break;

            if(displayCfg.transitionMode == SLOT_TRANS_MODE_SCROLL)
            {
                currentDigits[0] = (currentDigits[1] | 0x80);
                currentDigits[1] = currentDigits[2];
                currentDigits[2] = currentDigits[3];
                currentDigits[3] = SEGVALUE_OFF;
                
                blankDigitTransitionCounter = displayCfg.scrollBlanksCount;

                state = STATE_TRANSIT_BLANK;
            }
            else
            {
                currentDigits[0] = slots[activeSlot].digits[0];
                currentDigits[1] = slots[activeSlot].digits[1];
                currentDigits[2] = slots[activeSlot].digits[2];
                currentDigits[3] = slots[activeSlot].digits[3];
                
                if(displayCfg.showColon)
                {
                    currentDigits[0] &= 0x7F;
                    currentDigits[1] &= 0x7F;
                }
            }
        }
        break;
    case STATE_TRANSIT_BLANK:
        if(tickCounter >= displayCfg.scrollDuration)
        {
            if(blankDigitTransitionCounter > 0)
                blankDigitTransitionCounter--;
            
            currentDigits[0] = currentDigits[1];
            currentDigits[1] = currentDigits[2];
            currentDigits[2] = currentDigits[3];

            if(blankDigitTransitionCounter == 0)
            {
                currentDigits[3] = slots[activeSlot].digits[0];
                state = STATE_TRANSIT_DIG_1;
            }
            else
                currentDigits[3] = SEGVALUE_OFF;
                
            tickCounter = 0;
        }
        break;
    case STATE_TRANSIT_DIG_1:
        if(tickCounter >= displayCfg.scrollDuration)
        {
            currentDigits[0] = currentDigits[1];
            currentDigits[1] = currentDigits[2];
            currentDigits[2] = currentDigits[3];
            currentDigits[3] = slots[activeSlot].digits[1];
                
            tickCounter = 0;
            state = STATE_TRANSIT_DIG_2;
        }
        break;
    case STATE_TRANSIT_DIG_2:
        if(tickCounter >= displayCfg.scrollDuration)
        {
            currentDigits[0] = currentDigits[1];
            currentDigits[1] = currentDigits[2];
            currentDigits[2] = currentDigits[3];
            currentDigits[3] = slots[activeSlot].digits[2];
                
            tickCounter = 0;
            state = STATE_TRANSIT_DIG_3;
        }
        break;
    case STATE_TRANSIT_DIG_3:
        if(tickCounter >= displayCfg.scrollDuration)
        {
            currentDigits[0] = currentDigits[1];
            currentDigits[1] = currentDigits[2];
            currentDigits[2] = currentDigits[3];
            currentDigits[3] = slots[activeSlot].digits[3];
            
            if(displayCfg.showColon)
            {
                currentDigits[0] &= 0x7F;
                currentDigits[1] &= 0x7F;
            }
            
            tickCounter = 0;
            state = STATE_DISPLAY;
        }
        break;
    }
    
    tickCounter++;
}

void display_recalcPwmDutyCycle(void)
{
    onTimeCounter = ledPwmValues[displayCfg.brightnessInPercent];
    offTimeCounter = ledPwmValues[100] - onTimeCounter;
}

void display_setConfig(displayConfig_t* config)
{
    displayCfg = *config;
}

void display_setConfig_brightness(unsigned char percent)
{
    if(percent > 100)
        percent = 100;
    
    displayCfg.brightnessInPercent = percent;
}

void display_setSlotDisplayValue(unsigned char slotNumber, displayValue_t* value)
{
    if(slotNumber < NUMBER_OF_SLOTS)
    {
        slots[slotNumber] = *value;
        
        if(state == STATE_DISPLAY && slotNumber == activeSlot)
        {
            currentDigits[0] = slots[activeSlot].digits[0];
            currentDigits[1] = slots[activeSlot].digits[1];
            currentDigits[2] = slots[activeSlot].digits[2];
            currentDigits[3] = slots[activeSlot].digits[3];
            
            if(displayCfg.showColon)
            {
                currentDigits[0] &= 0x7F;
                currentDigits[1] &= 0x7F;
            }
        }
    }
}
