/*
 * File:   main.c
 * Author: seven
 *
 * Created on November 26, 2017, 10:06 PM
 */

// CONFIG
#pragma config FOSC = XT        // Oscillator Selection bits (XT oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config CP = OFF         // FLASH Program Memory Code Protection bit (Code protection off)
#pragma config BOREN = ON       // Brown-out Reset Enable bit (BOR enabled)


#include <xc.h>
#include "7segmentDisplay.h"



void main(void)
{
    
    display_init();
    
    //Timer2 Registers Prescaler= 16 - TMR2 PostScaler = 8 - PR2 = 156 - Freq = 50.08 Hz - Period = 0.019968 seconds
    T2CON |= 56; // bits 6-3 Post scaler 1:1 thru 1:16
    T2CONbits.TMR2ON = 1;
    T2CONbits.T2CKPS1 = 1;
    T2CONbits.T2CKPS0 = 0;
    PR2 = 156; // PR2 (Timer2 Match value)


    INTCON = 0;
    PIR1bits.TMR2IF = 0;
    PIE1bits.TMR2IE = 1;
    //INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;

    
    while(1)
    {
        display_handler();
    }

    return;
}


void interrupt isr(void)
{
    if(PIR1bits.TMR2IF == 1)
    {
        display_cyclicTasks();
        PIR1bits.TMR2IF = 0;
    }
}
