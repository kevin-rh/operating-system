/*
 * file: testparking.c
 */
#include <8051.h>
#include "preemptive.h"

__data __at (0x20) Semaphore mutex;
__data __at (0x21) Semaphore full;
__data __at (0x22) Semaphore empty;

__data __at (0x23) char parkInfo;
__data __at (0x24) char num;
__data __at (0x25) char car[MAXTHREADS];

__data __at (0x29) char tmp;

__data __at (0x2A) char slot1;
__data __at (0x2B) char slot2;
__data __at (0x2C) char avail;


const char Log[] = "(Log)\0";

const char Car[] = " CAR-\0";
const char Slot[] =" SLOT-\0";

const char Enter[] = " enters \0";
const char Leave[] = " leaves \0";
const char At[] = " at time \0";

#define cPrint(c)\
    SBUF = c;\
    TI = 0;\
    while(!TI);

void iPrint(char x){
    if(x>0x63){
        tmp = x/0x64;
        cPrint(tmp+'0');
    }
    if(x>0x09){
        tmp = x%0x64;
        tmp = x/0x0A;
        cPrint(tmp+'0');
    }
    tmp = x%10;
    cPrint(tmp+'0');
}

void sPrint(char* s){
    do{
        SBUF = *s;
        TI = 0;
        while(!TI);
        s++;
    }while(*s);
}

void log(char i){
    TMOD |= 0x20;        // Use only the TLx register as an 8-bit counter
    TH1 = (char) -6;    // Baud Rate: 4800
    SCON = 0x50;        // (Serial Port Mode: 8-bit standard UART) and (Enable Receive)
    TR1 = 1;            // Enable Timer <SFR-TCON>

    sPrint(Log);
    sPrint(Car);
    if (i%2==0){
        cPrint(slot1);
    }else{
        cPrint(slot2);
    } 
    sPrint(Slot);
    cPrint('1'+ (i%2));

    if(i/2==0){
        sPrint(Enter);
    }else{
        sPrint(Leave);
    }
    sPrint(At);
    iPrint(now());
    cPrint('\n');
}


void Producer(void){
    SemaphoreWait(empty);
    SemaphoreWait(mutex);
    EA = 0;
    if (slot1=='0'){
        slot1 = car[curThreadID];
        log(0);
    }else{
        slot2 = car[curThreadID];
        log(1);
    }
    EA = 1;
    SemaphoreSignal(mutex);
    delay(3);
    EA = 0;

    if (slot1 == car[curThreadID]){
        log(2);
        slot1 = 0;
    }else{
        log(3);
        slot2 = 0;
    }
    EA = 1;
    SemaphoreSignal(empty);
    SemaphoreSignal(full);
    ThreadExit();
}

void main(void){
    __critical{
        SemaphoreCreate(full,0);
        SemaphoreCreate(mutex,1);
        SemaphoreCreate(empty,2);
        num = '1';
        car[0]=car[1]=car[2]=car[3]='0';
        slot1 = slot2 = '0';
        parkInfo = 0x00;
    }

    for(tmp=0;tmp<3;tmp++){
        tmp = ThreadCreate(Producer);
        car[tmp] = num++;
    }

    while(now() <= 0x3F){
        SemaphoreWait(full);
        tmp = ThreadCreate(Producer);
        car[tmp] = num;
        num = (num - '0') % 5 + '1';
    }
    ThreadExit();
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
