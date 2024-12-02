#pragma once

#include <stdint.h>
#include <stddef.h>
#include <context.h>
#include <vmm.h>

#define THREAD_READY 1
#define THREAD_RUNNING 2
#define THREAD_SLEEPING 4
#define THREAD_BLOCKED 8

enum ThreadPriority {
    THREAD_LOW = 1,
    THREAD_MED = 2,
    THREAD_HIGH = 3
};

struct Proc;

typedef struct Thread {
    Context Ctx;
    PageMap *pPageMap;
    uint64_t Stack;
    uint64_t Priority;
    uint64_t Flags;
    uint32_t CpuNum;
    struct Thread *pNext;
    struct Thread *pPrev;
    struct Proc *pProc;
} Thread;

typedef struct {
    PageMap *pPageMap;
    int ID;
    struct Proc *pNext;
    struct Proc *pPrev;
    Thread *pThreads;
} Proc;

void KxSchedInit();
Thread *PsCreateThread(Proc* pProc, void *pEntry, uint64_t Priority, uint32_t CpuNum);
Proc *PsCreateProc();
Thread *PsGetThread();
void KxBlockThread();
