/*
 * file: testcoop.c
 */
#include <8051.h>
#include "cooperative.h"

__data __at(0x39) char sbuf;
__data __at(0x3A) int bufCount;

void Producer(void){
    sbuf = 'A'; // starts from 'A'
    while (1){

        if(bufCount == 1){
            ThreadYield();
        }else{
            bufCount++;
            ThreadYield();
            sbuf = (sbuf-'A'+1) % 26 + 'A'; // next character or starts from 'A' again
        }
    }
}
void Consumer(void){
    TMOD = 0x20;        // Use only the TLx register as an 8-bit counter
    TH1 = (char) -6;    // Baud Rate: 4800
    SCON = 0x50;        // (Serial Port Mode: 8-bit standard UART) and (Enable Receive)
    TR1 = 1;            // Enable Timer <SFR-TCON>

    TI = 1;             // Transmit Interrupt Flag = 1
    while (1){
        if (bufCount == 1){
            while(!TI);

            SBUF = sbuf;
            bufCount--;
            TI = 0 ;
        }
        ThreadYield();
    }
}
void main(void){
    bufCount = 0;
    ThreadCreate(Producer);
    Consumer();
}

void _sdcc_gsinit_startup(void){
    __asm
        ljmp  _Bootstrap
    __endasm;
}

void _mcs51_genRAMCLEAR(void) {}
void _mcs51_genXINIT(void) {}
void _mcs51_genXRAMCLEAR(void) {}
