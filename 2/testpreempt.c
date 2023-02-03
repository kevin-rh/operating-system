/*
 * file: testpreempt.c
 */
#include <8051.h>
#include "preemptive.h"

__data __at(0x38) char sbuf;
__data __at(0x39) int bufCount;

void Producer(void){
    sbuf = 'Z'; // starts from 'A'
    while (1){
        if(bufCount == 1){
            // Yield
        }else{
            __critical{
                sbuf = (sbuf-'A'+1) % 26 + 'A'; // next character or starts from 'A' again
                bufCount++;
            }
        }
    }
}

void Consumer(void){
    TMOD |= 0x20;        // Use only the TLx register as an 8-bit counter
    TH1 = (char) -6;    // Baud Rate: 4800
    SCON = 0x50;        // (Serial Port Mode: 8-bit standard UART) and (Enable Receive)
    TR1 = 1;            // Enable Timer <SFR-TCON>

    TI = 1;             // Transmit Interrupt Flag = 1
    while (1){
        if (bufCount == 1){
            while(!TI); // Yield
            __critical{
                SBUF = sbuf;
                bufCount--;
                TI = 0 ;
            }
        }
    }
}

void main(void){
    __critical{
        bufCount = 0;
    }
    ThreadCreate(Consumer);
    Producer();

}

void _sdcc_gsinit_startup(void){
    __asm
        ljmp  _Bootstrap
    __endasm;
}

void _mcs51_genRAMCLEAR(void) {}
void _mcs51_genXINIT(void) {}
void _mcs51_genXRAMCLEAR(void) {}

void timer0_ISR(void) __interrupt(1) {
    __asm
        ljmp _myTimer0Handler
    __endasm;
}
