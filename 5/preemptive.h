/*
 * file: preemptive.h
 *
 * this is the include file for the preemptive multithreading
 * package.  It is to be compiled by SDCC and targets the EdSim51 as
 * the target architecture.W
 *
 * CS 3423 Fall 2018
 */

#ifndef __PREEMPTIVE_H__
#define __PREEMPTIVE_H__

#define MAXTHREADS 4  /* not including the scheduler */
/* the scheduler does not take up a thread of its own */

typedef char ThreadID;
typedef char Semaphore;
typedef void (*FunctionPtr)(void);

__data __at (0x3C) ThreadID curThreadID;  // Current Thread

ThreadID ThreadCreate(FunctionPtr);
void ThreadYield(void);
void ThreadExit(void);

#define CNAME(s) _ ## s
#define LABEL(label) label ## $
#define SemaphoreCreate(s, n)  s = n
#define SemaphoreWait(s) SemaphoreWaitBody(s, __COUNTER__)
#define SemaphoreSignal(s) {\
    __asm\
        INC CNAME(s)\
    __endasm;\
}
#define SemaphoreWaitBody(S, label) \
    {   __asm \
        LABEL(label): \
        MOV A, CNAME(S) \
        JZ LABEL(label) \
        JB ACC.7, LABEL(label) \
        dec CNAME(S) \
        __endasm; \
    }

void delay(unsigned char);
unsigned char now(void);

#endif // __PREEMPTIVE_H__
