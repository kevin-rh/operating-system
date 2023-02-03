/*
 * file: cooperative.c
 */
#include <8051.h>

#include "cooperative.h"

// data at 0x30 - 0x33 := reserved for savedSP[0:3]
__data __at (0x30) char savedSP[MAXTHREADS];
__data __at (0x34) char threadBitmap;   // Thread "counter"
__data __at (0x35) ThreadID curThreadID;  // Current Thread
__data __at (0x36) char threadPTR;      // Initialized to savedSP's address
__data __at (0x37) ThreadID initThread; // temporary in CreateThread
__data __at (0x38) char tempSP;         // temporary save the SP

#define SAVESTATE {\
    __asm \
        PUSH ACC\
        PUSH B\
        PUSH DPL\
        PUSH DPH\
        PUSH PSW\
    __endasm; \
    savedSP[curThreadID] = SP;\
}

#define RESTORESTATE {\
    SP = savedSP[curThreadID];\
    __asm \
        POP PSW\
        POP DPH\
        POP DPL\
        POP B\
        POP ACC\
    __endasm; \
}

extern void main(void);

void Bootstrap(void){
    threadBitmap = 0b0000;  // Set to no active thread 
    curThreadID = 0;        // Set to NULL
    threadPTR = 0x30;       // Set Pointer to PTR's address
    // (1) SP = 0x07;
    curThreadID = ThreadCreate(main); // (2) (3)
    RESTORESTATE;   // (4)
}

ThreadID ThreadCreate(FunctionPtr fp){
    if ((threadBitmap & 0b1111) == 0b1111) return -1; // Reached max #thread 
    // c) save currect SP into temporary set SP to the starting location for the new thread
    tempSP = SP;    

    // a) update the bit mask
    // b) calculate the starting stack location for new thread
    if       ((threadBitmap & 0b0001) == 0b0000) { // check map0
        threadBitmap |= 0b0001; // update
        initThread = 0;
        SP = 0x3F; // starting stack
        __asm // 10
            MOV PSW, #0x00
        __endasm;
    }else if ((threadBitmap & 0b0010) == 0b0000) { // check map1
        threadBitmap |= 0b0010; // update
        initThread = 1;
        SP = 0x4F; // starting stack
        __asm // 01
            MOV PSW, #0x08
        __endasm;
    }else if ((threadBitmap & 0b0100) == 0b0000) { // check map2
        threadBitmap |= 0b0100; // update
        initThread = 2;
        SP = 0x5F; // starting stack
        __asm // 10
            MOV PSW, #0x10
        __endasm;
    }else                                        { // check map3
        threadBitmap |= 0b1000; // update
        initThread = 3;
        SP = 0x6F; // starting stack
        __asm // 11
            MOV PSW, #0x18
        __endasm;
    }

    // d) push the return address fp 
    __asm
        PUSH DPL
        PUSH DPH
    __endasm;

    // e) push 0s 4 times for ACC, B, DPL, DPH.
    __asm
        MOV A, #0x00
        PUSH A // ACC
        PUSH A // B
        PUSH A // DPL
        PUSH A // DPH
    __endasm;

    // f) push PSW: CY AC F0 RS1 RS0 OV UD P
    __asm
        PUSH PSW
    __endasm;

    // g) write the current stack pointer to the saved stack pointer array for this newly created thread ID
    savedSP[initThread] = SP;

    // h) set SP to the saved SP in step (c)
    SP = tempSP;

    // i) finally, return the newly created thread ID
    return initThread; 
}

void ThreadYield(void){
    SAVESTATE;
    do{
        if ((threadBitmap & 0b1111) == 0b0000) break; // No thread running
        curThreadID = (curThreadID + 1) % 4;
        if ((threadBitmap & (0b0001<<curThreadID))!= 0b0000) break; // Exist thread
    }
    while (1);
    RESTORESTATE;
}


void ThreadExit(void){
    threadBitmap &= ~(0b0001<<(curThreadID));
    do{
        if ((threadBitmap & 0b1111) == 0b0000) break; // No thread running
        curThreadID = (curThreadID + 1) % 4;
        if ((threadBitmap & (0b0001<<curThreadID))!= 0b0000) break; // Exist thread
    }
    while (1);
    RESTORESTATE;
}

