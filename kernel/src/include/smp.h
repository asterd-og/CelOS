#pragma once

#include <stdint.h>
#include <stddef.h>
#include <vmm.h>
#if defined (__x86_64__)
#include <x86/smp.h>
#endif
#include <sched.h>

typedef struct {
    ArchCpuInfo ArchInfo;
    PageMap *pCurrentPageMap;
    uint64_t CpuNum;
    Proc *pProcList;

    Proc *pCurrentProc;
    Thread *pCurrentThread;
} CpuInfo;

void KeSmpInit();
CpuInfo *KeSmpGetCpu();
CpuInfo *KeSmpGetCpuByNum(uint32_t Num);
