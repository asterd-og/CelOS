#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <alloc.h>
#include <vmm.h>
#if defined (__x86_64__)
#include <x86/smp.h>
#endif
#include <sched.h>
#include <list.h>

typedef struct TaskQueue {
    bool HasRunnableTask;
    uint64_t Priority;
    List *pTasks;
    ListItem *pIterator;
    struct TaskQueue *pNext;
} TaskQueue;

typedef struct {
    ArchCpuInfo ArchInfo;
    PageMap *pCurrentPageMap;
    uint64_t CpuNum;
    AllocatorDescriptor *pCurrentAllocator;

    uint8_t IPL;
    uint8_t QueuedIrqs[256];
    uint8_t QueuedIrqIdx;
    bool RunningIrq;

    TaskQueue *pTaskQueue;
    Task *pCurrentTask;
} CpuInfo;

extern bool g_SmpStarted;
extern AllocatorDescriptor *g_pKernelAllocator;

void KeSmpInit();
CpuInfo *KeSmpGetCpu();
CpuInfo *KeSmpGetCpuByNum(uint32_t Num);
AllocatorDescriptor *KeSmpSwitchAllocator(AllocatorDescriptor *pAllocator);
