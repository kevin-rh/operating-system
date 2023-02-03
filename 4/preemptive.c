/*
 * file: preemptive.c
 */
#include <8051.h>

#include "preemptive.h"

// data at 0x30 - 0x33 := reserved for savedSP[0:3]
__data __at (0x30) char savedSP[MAXTHREADS];
__data __at (0x34) char threadBitmap;   // Thread "counter"
__data __at (0x35) ThreadID curThreadID;  // Current Thread
__data __at (0x36) ThreadID tempThreadID; // temporary in CreateThread
__data __at (0x37) char temp;         // temporary save the SP and isFoundThread 
__data __at (0x38) char turn;

#define SAVESTATE {\
    __asm \
        PUSH ACC\
        PUSH B\
        PUSH DPL\
        PUSH DPH\
        PUSH PSW\
        MOV ACC, R0\
        PUSH ACC\
        MOV	A,_curThreadID\
	    ADD	A,#_savedSP\
	    MOV	R0,A\
	    MOV	@R0,_SP\
        POP ACC\
        MOV R0, ACC\
    __endasm; \
}

#define RESTORESTATE {\
    __asm \
        MOV ACC, R1\
        PUSH ACC\
        mov	a,_curThreadID\
        add	a,#_savedSP\
        mov	r1,a\
        mov	_SP,@r1\
        POP ACC\
        MOV R1, ACC\
        POP PSW\
        POP DPH\
        POP DPL\
        POP B\
        POP ACC\
    __endasm; \
}

extern void main(void);

void Bootstrap(void){
    turn = 1;
    TMOD = 0;  // timer 0 mode 0
    IE = 0x82;  // enable timer 0 interrupt; keep consumer polling
                // EA  -  ET2  ES  ET1  EX1  ET0  EX0
    TR0 = 1; // set bit TR0 to start running timer 0

    threadBitmap = 0b0000;  // Set to no active thread 
    curThreadID = ThreadCreate(main); // Initial Thread Creation
    RESTORESTATE; // Run Initial Thread
}

ThreadID ThreadCreate(FunctionPtr fp){
    if ((threadBitmap & 0b1111) == 0b1111) return -1; // Reached max #thread

    EA = 0; // Enter Critical Section, since weird behavior occured

    temp = SP; // Save currect SP into temporary  
      
    // Update the bit mask and GET NEW Thread's SP and BANK
    if       ((threadBitmap & 0b0001) == 0b0000) { // check map0
        threadBitmap |= 0b0001; // update
        tempThreadID = 0;
        SP = 0x3F;  // SET NEW Starting Stack
        __asm       // 10
            MOV PSW, #0x00
        __endasm;
    }else if ((threadBitmap & 0b0010) == 0b0000) { // check map1
        threadBitmap |= 0b0010; // update
        tempThreadID = 1;
        SP = 0x4F;  // SET NEW Starting Stack
        __asm       // 01
            MOV PSW, #0x08
        __endasm;
    }else if ((threadBitmap & 0b0100) == 0b0000) { // check map2
        threadBitmap |= 0b0100; // update
        tempThreadID = 2;
        SP = 0x5F;  // SET NEW Starting Stack
        __asm       // 10
            MOV PSW, #0x10
        __endasm;
    }else                                        { // check map3
        threadBitmap |= 0b1000; // update
        tempThreadID = 3;
        SP = 0x6F;  // SET NEW Starting Stack
        __asm       // 11
            MOV PSW, #0x18
        __endasm;
    }

    // Push the return address fp and 0s for...
    __asm
        PUSH DPL
        PUSH DPH
        MOV A, #0x00
        PUSH A // ACC
        PUSH A // B
        PUSH A // DPL
        PUSH A // DPH
    __endasm;

    // Push PSW: CY AC F0 RS1 RS0 OV UD P
    __asm
        PUSH PSW
    __endasm;

    // Write the current stack pointer to the saved stack pointer array for this newly created thread ID
    __asm
        MOV ACC, R0
        PUSH ACC
        MOV	A,_tempThreadID
        ADD	A,#_savedSP
        MOV	R0,A
        MOV	@R0,_SP
        POP ACC
        MOV R0, ACC
    __endasm;

    SP = temp; // Set SP to the saved temp

    EA=1; // Exit Critical Section

    return tempThreadID; 
}

void ThreadYield(void){
    __critical{ // Enter Critical Section
        SAVESTATE;
        do{
            if ((&threadBitmap) == 0) break; // No thread running
            curThreadID = (curThreadID + 1) % 4;
            temp = threadBitmap & (1<<curThreadID); // Exist thread
            if(temp) break;
        }
        while (1);
        RESTORESTATE;
    } // Exit Critical Section
}


void ThreadExit(void){
    __critical{ // Enter Critical Section
        threadBitmap &= ~(0b0001<<(curThreadID));
        do{
            if ((&threadBitmap) == 0) break; // No thread running
            curThreadID = (curThreadID + 1) % 4;
            temp = threadBitmap & (1<<curThreadID); // Exist thread
            if(temp) break;
        }
        while (1);
        RESTORESTATE;
    } // Exit Critical Section
}

void myTimer0Handler(void) {
    EA = 0; // Enter Critical Section
    SAVESTATE;
    do{
        if ((&threadBitmap) == 0) break; // No thread running
    
        if(curThreadID==0){
            curThreadID = (turn) ? 1 : 2;
            turn = !turn;
        }else curThreadID = 0;

        temp = threadBitmap & (1<<curThreadID); // Exist thread
        if(temp) break;
    }
    while (1);

    RESTORESTATE;
    EA = 1; // Exit Critical Section

    __asm
        RETI
    __endasm;
}



