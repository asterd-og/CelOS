#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <vmm.h>
#if defined (__x86_64__)
#include <x86/smp.h>
#endif
#include <sched.h>

typedef struct ThreadQueue {
    bool HasRunnableThread;
    uint64_t Priority;
    struct Thread *pThreads;
    struct ThreadQueue *pNext;
} ThreadQueue;

typedef struct {
    ArchCpuInfo ArchInfo;
    PageMap *pCurrentPageMap;
    uint64_t CpuNum;

    uint8_t IPL;
    uint8_t QueuedIrqs[256];
    uint8_t QueuedIrqIdx;
    bool RunningIrq;

    ThreadQueue *pThreadQueue;

    Thread *pCurrentThread;
} CpuInfo;

void KeSmpInit();
CpuInfo *KeSmpGetCpu();
CpuInfo *KeSmpGetCpuByNum(uint32_t Num);
