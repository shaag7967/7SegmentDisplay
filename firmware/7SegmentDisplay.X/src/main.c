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

// *** defines *****************************************************************
#define I2C_ADDRESS     0x70  // 7 bit address (bit 0 to 6)

#define DISPLAY_CMD_CONFIG      0x01

// *** functions ***************************************************************
void i2c_init(char address);
void timer2_init(void);

// *** variables ***************************************************************
char rxByteCount = 0;
char rxDataBuffer[10];
char rxDataAvailable = 0;
char transferActive= 0;



void main(void)
{
    i2c_init(I2C_ADDRESS);
    timer2_init();

    display_init();
    
    
    INTCON = 0;
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;

    TRISC1 = 0;
    RC1 = 0;
    TRISC2 = 0;
    RC2 = 0;
    TRISC5 = 0;
    RC5 = 0;    
    TRISC6 = 0;
    RC6 = 0;
    TRISC7 = 0;
    RC7 = 0;
    
    RC6 = SSPSTATbits.BF;
    
    while(1)
    {
        display_handler();

        if(rxDataAvailable)
        {
            INTCONbits.GIE = 0;
            
            if(rxByteCount == 9)
            {
                if(rxDataBuffer[1] == DISPLAY_CMD_CONFIG)
                {
                    char* dataPtr = (char*)&display;
                    dataPtr[0] = rxDataBuffer[2];
                    dataPtr[1] = rxDataBuffer[3];
                    dataPtr[2] = rxDataBuffer[4];
                    dataPtr[3] = rxDataBuffer[5];
                    dataPtr[4] = rxDataBuffer[6];
                    dataPtr[5] = rxDataBuffer[7];
                    dataPtr[6] = rxDataBuffer[8];
                    
                    if(display.brightness == 56)
                        RC2 = 1;
                }
            }
            
            rxDataAvailable = 0;
            INTCONbits.GIE = 1;
        }
    }

    return;
}


void i2c_init(char address)
{
    SSPSTAT = 0x00;
    SSPADD = (unsigned char)(address << 1);
    SSPCON = 0x3E;

    TRISC3 = 1;
    TRISC4 = 1;

    PIR1bits.SSPIF = 0;
    PIE1bits.SSPIE = 1;
}

void timer2_init(void)
{
    //Timer2 Registers Prescaler= 16 - TMR2 PostScaler = 8 - PR2 = 156 - Freq = 50.08 Hz - Period = 0.019968 seconds
    T2CON |= 56; // bits 6-3 Post scaler 1:1 thru 1:16
    T2CONbits.TMR2ON = 1;
    T2CONbits.T2CKPS1 = 1;
    T2CONbits.T2CKPS0 = 0;
    PR2 = 156; // PR2 (Timer2 Match value)
    
    PIR1bits.TMR2IF = 0;
    PIE1bits.TMR2IE = 1;
}

char dummy;

void interrupt isr(void)
{
    if(PIR1bits.SSPIF == 1)
    {
        SSPCONbits.CKP = 0;
        
        if((SSPCONbits.SSPOV) || (SSPCONbits.WCOL))
        {
            dummy = SSPBUF; // Read the previous value to clear the buffer
            SSPCONbits.SSPOV = 0; // Clear the overflow flag
            SSPCONbits.WCOL = 0; // Clear the collision bit
        }
        
        if(transferActive == 0)
        {
            if(SSPSTATbits.S == 1 && SSPSTATbits.P == 0) // start bit
            {
                transferActive = 1;
                rxByteCount = 0;
            }
        }
        else
        {
            if(SSPSTATbits.R_nW == 0) // write
            {
                rxDataBuffer[rxByteCount] = SSPBUF;
                rxByteCount++;
            }
            else // read
            {
                SSPBUF = 0x77;
            }
            
            if(SSPSTATbits.S == 0 && SSPSTATbits.P == 1) // stop bit
            {
                transferActive = 0;
                rxDataAvailable = 1;
            }
        }
        
        SSPCONbits.CKP = 1; // release clock
        PIR1bits.SSPIF = 0;
    }

    if(PIR1bits.TMR2IF == 1)
    {
        display_cyclicTasks();
        PIR1bits.TMR2IF = 0;
    }
}
