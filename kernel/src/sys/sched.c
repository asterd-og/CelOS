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
#include <assert.h>

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

Thread *PsCreateThread(Proc* pProc, void *pEntry, uint64_t Priority, uint32_t CpuNum) {
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
    pThread->pProc = pProc;
    pThread->CpuNum = CpuNum;
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
    // Put it in the appropriate queue
    CpuInfo *pCpu = KeSmpGetCpuByNum(CpuNum);
    ThreadQueue *pQueue = NULL;
    switch (Priority) {
        case THREAD_HIGH:
            pQueue = pCpu->pThreadQueue;
            break;
        case THREAD_MED:
            pQueue = pCpu->pThreadQueue->pNext;
            break;
        case THREAD_LOW:
        default:
            pQueue = pCpu->pThreadQueue->pNext->pNext;
            break;
    }
    pQueue->HasRunnableThread = true;
    if (!pQueue->pThreads) {
        pThread->pNext = pThread;
        pThread->pPrev = pThread;
        pQueue->pThreads = pThread;
        return pThread;
    }
    pThread->pNext = pQueue->pThreads;
    pThread->pPrev = pQueue->pThreads->pPrev;
    pQueue->pThreads->pPrev->pNext = pThread;
    pQueue->pThreads->pPrev = pThread;
    return pThread;
}

Proc *PsCreateProc() {
    Proc *pProc = (Proc*)MmAlloc(sizeof(Proc));
    pProc->pPageMap = MmNewPageMap();
    pProc->ID = g_PID++;
    pProc->pThreads = NULL;
    return pProc;
}

void KxSchedule(Context *pCtx) {
    CpuInfo *pCpu = KeSmpGetCpu();
    Thread *pThread = pCpu->pCurrentThread;
    if (pThread) {
        memcpy(&pThread->Ctx, pCtx, sizeof(Context));
        pThread->Flags &= ~THREAD_RUNNING;
    }
    // Find highest queue to run
    ThreadQueue *pQueue = pCpu->pThreadQueue;
    while (!pQueue->pThreads || !pQueue->HasRunnableThread) {
        if (pQueue->pNext == NULL) {
            KeLocalApicEoi();
            KeLocalApicOneShot(32 + g_SchedVector, QUANTUM * 5);
            return;
        }
        pQueue = pQueue->pNext;
    }
    pThread = pQueue->pThreads;
    while (!(pThread->Flags & THREAD_READY)) {
        pThread = pThread->pNext;
        ASSERT(pThread != pQueue->pThreads);
    }
    pCpu->pCurrentThread = pThread;
    pThread->Flags |= THREAD_RUNNING;
    memcpy(pCtx, &pThread->Ctx, sizeof(Context));
    MmSwitchPageMap(pThread->pPageMap);
    KeLocalApicEoi();
    KeLocalApicOneShot(32 + g_SchedVector, QUANTUM * pThread->Priority);
}

Thread *PsGetThread() {
    return KeSmpGetCpu()->pCurrentThread;
}

void KxBlockThread() {
    Thread *pThread = PsGetThread();
    pThread->Flags &= ~(THREAD_RUNNING | THREAD_READY);
    pThread->Flags |= THREAD_BLOCKED;
    CpuInfo *pCpu = KeSmpGetCpuByNum(pThread->CpuNum);
    ThreadQueue *pQueue = NULL;
    switch (pThread->Priority) {
        case THREAD_HIGH:
            pQueue = pCpu->pThreadQueue;
            break;
        case THREAD_MED:
            pQueue = pCpu->pThreadQueue->pNext;
            break;
        case THREAD_LOW:
        default:
            pQueue = pCpu->pThreadQueue->pNext->pNext;
            break;
    }
    bool FoundRunnable = false;
    Thread *pQueueThread = pQueue->pThreads;
    while (pQueueThread != pQueue->pThreads) {
        if (pQueueThread->Flags & THREAD_READY || pQueueThread->Flags & THREAD_RUNNING) {
            FoundRunnable = true;
            break;
        }
        pQueueThread = pQueueThread->pNext;
    }
    pQueue->HasRunnableThread = FoundRunnable;
    return;
}
