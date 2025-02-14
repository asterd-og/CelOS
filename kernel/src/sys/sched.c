#include <sched.h>
#include <pmm.h>
#include <smp.h>
#include <string.h>
#include <interrupt.h>
#include <printf.h>
#include <assert.h>

#define QUANTUM 5

uint64_t g_ID = 0;
uint8_t g_SchedVector = 0;

bool g_SchedInitialised = false;

void KxSchedule(Context *pCtx);

void KxSchedInit() {
    g_SchedVector = KxGetFreeIrq();
    KxInstallIrq(g_SchedVector, KxSchedule, 1);
    g_SchedInitialised = true;
    KxSendInt(1, g_SchedVector);
    ///KxSendIntAll(g_SchedVector);
}

TaskQueue *KxGetQueueFromPriority(uint64_t Priority, CpuInfo *pCpu) {
    TaskQueue *pQueue = NULL;
    switch (Priority) {
        case TASK_HIGH:
            pQueue = pCpu->pTaskQueue;
            break;
        case TASK_MED:
            pQueue = pCpu->pTaskQueue->pNext;
            break;
        case TASK_LOW:
        default:
            pQueue = pCpu->pTaskQueue->pNext->pNext;
            break;
    }
    return pQueue;
}

Task *KxCreateTask(void *pEntry, uint64_t Priority, uint32_t CpuNum) {
    // TODO: Check if CPU exists.
    CpuInfo *pCpu = KeSmpGetCpuByNum(CpuNum);

    Task *pTask = (Task*)MmAlloc(sizeof(Task));
    pTask->ID = g_ID++;
    pTask->pPageMap = MmNewPageMap();

    pTask->Stack = MmVirtAllocatePages(g_pKernelPageMap, 3, MM_READ | MM_WRITE);

    CTX_IP(pTask->Ctx) = (uint64_t)pEntry;
    CTX_STK(pTask->Ctx) = pTask->Stack + (PAGE_SIZE * 3);
    CTX_SEG(pTask->Ctx);
    CTX_FLAGS(pTask->Ctx);

    pTask->Flags = TASK_READY;

    TaskQueue *pQueue = KxGetQueueFromPriority(Priority, pCpu);
    pQueue->HasRunnableTask = true;
    ListAppend(pQueue->pTasks, pTask);

    pTask->Priority = Priority;

    return pTask;
}

void KxSchedule(Context *pCtx) {
    KxPauseTimer();
    CpuInfo *pCpu = KeSmpGetCpu();
    Task *pTask = pCpu->pCurrentTask;
    if (pTask) {
        pTask->Ctx = *pCtx;
        if (pTask->Flags == TASK_RUNNING)
            pTask->Flags = TASK_READY;
    }
    // Find highest queue to run.
    TaskQueue *pQueue = pCpu->pTaskQueue;
    while (pQueue->pTasks->Count == 0 || !pQueue->HasRunnableTask) {
        if (pQueue->pNext == NULL) {
            KxEndOfInt();
            KxTimeInt(g_SchedVector, QUANTUM * 5);
            return;
        }
        pQueue = pQueue->pNext;
    }
    while (true) {
        pQueue->pIterator = pQueue->pIterator->pNext;
        if (pQueue->pIterator == pQueue->pTasks->pHead)
           pQueue->pIterator = pQueue->pIterator->pNext;
        pTask = (Task*)pQueue->pIterator->pData;
        if (pTask->Flags == TASK_READY) {
            break;
        }
    }
    pCpu->pCurrentTask = pTask;
    pTask->Flags = TASK_RUNNING;

    *pCtx = pTask->Ctx;
    MmSwitchPageMap(pTask->pPageMap);

    KxEndOfInt();
    KxTimeInt(g_SchedVector, QUANTUM * pTask->Priority);
}

void KxBlockSched() {
    KxPauseTimer();
}

void KxUnblockSched() {
    KxSendInt(KeSmpGetCpu()->CpuNum, g_SchedVector);
}
