#include <sched.h>
#include <alloc.h>
#include <pmm.h>
#include <smp.h>
#include <string.h>
#if defined (__x86_64__)
#include <x86/idt.h>
#include <x86/apic.h>
#endif
#include <interrupt.h>
#include <printf.h>

#define QUANTUM 5

uint64_t g_PID = 0;
uint8_t g_SchedVector = 0;

void KxSchedule(Context *pCtx);

void KxSchedInit() {
    g_SchedVector = KxGetFreeIrq();
    KxInstallIrq(g_SchedVector, KxSchedule, 1);
    #if defined (__x86_64__)
    KeLocalApicIpiAll(0, 32 + g_SchedVector);
    #endif
}

Thread *PsCreateThread(Proc *pProc, void *pEntry, uint64_t Priority) {
    Thread *pThread = (Thread*)MmAlloc(sizeof(Thread));
    pThread->Stack = (uint64_t)HIGHER_HALF(MmPhysAllocatePage());
    CTX_IP(pThread->Ctx) = (uint64_t)pEntry;
    CTX_STK(pThread->Ctx) = pThread->Stack + PAGE_SIZE;
    pThread->Ctx.CS = 0x08;
    pThread->Ctx.SS = 0x10;
    pThread->Ctx.RFlags = 0x202;
    pThread->pPageMap = pProc->pPageMap;
    pThread->Priority = Priority;
    pThread->Flags = THREAD_READY;
    if (!pProc->pThreads) {
        pProc->pThreads = pThread;
        pThread->pPrev = pThread;
        pThread->pNext = pThread;
    } else {
        pThread->pNext = pProc->pThreads;
        pThread->pPrev = pProc->pThreads->pPrev;
        pProc->pThreads->pPrev->pNext = pThread;
        pProc->pThreads->pPrev = pThread;
    }
    return pThread;
}

Proc *PsCreateProcOnCpu(uint32_t CpuNum) {
    Proc *pProc = (Proc*)MmAlloc(sizeof(Proc));
    pProc->pPageMap = MmNewPageMap();
    pProc->ID = g_PID++;
    CpuInfo *pCpu = KeSmpGetCpuByNum(CpuNum);
    if (!pCpu->pProcList) {
        pCpu->pProcList = pProc;
        pProc->pNext = pProc;
        pProc->pPrev = pProc;
        pCpu->pCurrentProc = pProc;
    } else {
        pProc->pNext = pCpu->pProcList;
        pProc->pPrev = pCpu->pProcList->pPrev;
        pCpu->pProcList->pPrev->pNext = pProc;
        pCpu->pProcList->pPrev = pProc;
    }
    pProc->pThreads = NULL;
    return pProc;
}

Proc *PsCreateProc() {
    Proc *pProc = PsCreateProcOnCpu(KeSmpGetCpu()->CpuNum);
    return pProc;
}

void KxSchedule(Context *pCtx) {
    CpuInfo *pCpu = KeSmpGetCpu();
    if (!pCpu->pCurrentProc) {
        KeLocalApicEoi();
        KeLocalApicOneShot(32 + g_SchedVector, QUANTUM * 5);
        return;
    }
    Proc *pProc = pCpu->pCurrentProc;
    Thread *pThread = pCpu->pCurrentThread;
    if (!pThread) {
        while (!pProc->pThreads) {
            pProc = pProc->pNext;
            if (pProc == pCpu->pCurrentProc) {
                KeLocalApicEoi();
                KeLocalApicOneShot(32 + g_SchedVector, QUANTUM * 5);
                return;
            }
        }
        pThread = pProc->pThreads;
    }
    if (pThread->Flags & THREAD_RUNNING) {
        memcpy(&pThread->Ctx, pCtx, sizeof(Context));
        pThread->Flags &= ~THREAD_RUNNING;
        pThread = pThread->pNext;
        if (pThread == pProc->pThreads) {
            pProc = pProc->pNext;
            while (!pProc->pThreads) {
                pProc = pProc->pNext;
                if (pProc == pCpu->pCurrentProc) {
                    KeLocalApicEoi();
                    KeLocalApicOneShot(32 + g_SchedVector, QUANTUM * 5);
                    return;
                }
            }
            pThread = pProc->pThreads;
        }
    }
    pThread->Flags |= THREAD_RUNNING;
    pCpu->pCurrentThread = pThread;
    pCpu->pCurrentProc = pProc;
    memcpy(pCtx, &pThread->Ctx, sizeof(Context));
    KeLocalApicEoi();
    KeLocalApicOneShot(32 + g_SchedVector, QUANTUM * pThread->Priority);
    MmSwitchPageMap(pThread->pPageMap);
}
