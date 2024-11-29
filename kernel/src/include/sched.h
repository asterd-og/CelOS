#pragma once

#include <stdint.h>
#include <stddef.h>
#include <context.h>
#include <vmm.h>

#define THREAD_READY 1
#define THREAD_RUNNING 2

enum ThreadPriority {
    THREAD_LOW = 1,
    THREAD_MED = 2,
    THREAD_HIGH = 4
};

typedef struct Thread {
    Context Ctx;
    PageMap *pPageMap;
    uint64_t Stack;
    uint64_t Priority;
    uint64_t Flags;
    struct Thread *pNext;
    struct Thread *pPrev;
} Thread;

typedef struct Proc {
    PageMap *pPageMap;
    int ID;
    struct Proc *pNext;
    struct Proc *pPrev;
    Thread *pThreads;
} Proc;

void KxSchedInit();
Thread *PsCreateThread(Proc* pProc, void *pEntry, uint64_t Priority);
Proc *PsCreateProcOnCpu(uint32_t CpuNum);
Proc *PsCreateProc();
