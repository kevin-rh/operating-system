/*
 * file: test3threads.c
 */
#include <8051.h>
#include "preemptive.h"

__data __at(0x28) char chr;
__data __at(0x29) char num;

__data __at (0x20) Semaphore mutex;
__data __at (0x21) Semaphore full;
__data __at (0x22) Semaphore empty;

__data __at (0x23) char head;
__data __at (0x24) char tail;
__data __at (0x25) char ibuf[3];


void Producer(void){
    chr = 'A'; // starts from 'A'
    while (1){
        SemaphoreWait(empty);
        SemaphoreWait(mutex);
        __critical{
            ibuf[head] = chr;
            head = (head + 1) % 3;
            chr = (chr-'A'+1) % 26 + 'A'; // next character or starts from 'A' again
        }
        SemaphoreSignal(mutex);
        SemaphoreSignal(full);
    }
}

void ProducerNum(void){
    num = '0'; // starts from 'A'
    while (1){
        SemaphoreWait(empty);
        SemaphoreWait(mutex);
        __critical{
            ibuf[head] = num;
            head = (head + 1) % 3;
            num = (num-'0'+1) % 10 + '0'; // next character or starts from 'A' again
        }
        SemaphoreSignal(mutex);
        SemaphoreSignal(full);
    }
}

void Consumer(void){
    TMOD |= 0x20;        // Use only the TLx register as an 8-bit counter
    TH1 = (char) -6;    // Baud Rate: 4800
    SCON = 0x50;        // (Serial Port Mode: 8-bit standard UART) and (Enable Receive)
    TR1 = 1;            // Enable Timer <SFR-TCON>
    TI = 1;             // Transmit Interrupt Flag = 1
    
    while (1){
        SemaphoreWait(full);
        SemaphoreWait(mutex);
        while(!TI); // Yield
        __critical{
            SBUF = ibuf[tail];
            TI = 0 ;
            tail = (tail + 1) % 3;
        }
        SemaphoreSignal(mutex);
        SemaphoreSignal(empty);
    }
}

void main(void){
    __critical{
        head = tail = 0;
        ibuf[0] = ibuf[1] = ibuf[2] = 0;
        SemaphoreCreate(full,0);
        SemaphoreCreate(mutex,1);
        SemaphoreCreate(empty,3);
    }
    ThreadCreate(Producer);
    ThreadCreate(ProducerNum);
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

void timer0_ISR(void) __interrupt(1) {
    __asm
        ljmp _myTimer0Handler
    __endasm;
}
