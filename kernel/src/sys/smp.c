#include <smp.h>
#include <limine.h>
#include <alloc.h>
#include <spinlock.h>
#include <printf.h>
#include <pmm.h>
#include <string.h>

__attribute__((used, section(".limine_requests")))
static volatile struct limine_smp_request SmpRequest = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0
};

CpuInfo *g_pSmpCpuList;
SpinLock g_SmpLock;
uint64_t g_SmpStartedCount = 0;

bool g_SmpStarted = false;

void KeSmpCreateTaskQueue(CpuInfo *pCpu) {
    TaskQueue *pTaskHighQueue = (TaskQueue*)MmAlloc(sizeof(TaskQueue));
    TaskQueue *pTaskMedQueue = (TaskQueue*)MmAlloc(sizeof(TaskQueue));
    TaskQueue *pTaskLowQueue = (TaskQueue*)MmAlloc(sizeof(TaskQueue));
    pTaskHighQueue->pTasks = ListCreate();
    pTaskHighQueue->Priority = TASK_HIGH;
    pTaskHighQueue->pNext = pTaskMedQueue;
    pTaskHighQueue->HasRunnableTask = false;
    pTaskHighQueue->pIterator = pTaskHighQueue->pTasks->pHead;

    pTaskMedQueue->pTasks = ListCreate();
    pTaskMedQueue->Priority = TASK_MED;
    pTaskMedQueue->pNext = pTaskLowQueue;
    pTaskMedQueue->HasRunnableTask = false;
    pTaskMedQueue->pIterator = pTaskMedQueue->pTasks->pHead;

    pTaskLowQueue->pTasks = ListCreate();
    pTaskLowQueue->Priority = TASK_LOW;
    pTaskLowQueue->pNext = NULL;
    pTaskLowQueue->HasRunnableTask = false;
    pTaskLowQueue->pIterator = pTaskLowQueue->pTasks->pHead;

    pCpu->pTaskQueue = pTaskHighQueue;
}

void KeSmpCpuInit(struct limine_smp_info *pSmpInfo) {
    SpinLockAcquire(&g_SmpLock);
    CpuInfo *pCpu = (CpuInfo*)pSmpInfo->extra_argument;
    MmSwitchPageMap(g_pKernelPageMap);
    pCpu->pCurrentPageMap = g_pKernelPageMap;

    KeArchSmpCpuInit(pSmpInfo, &pCpu->ArchInfo);
    pCpu->CpuNum = KeArchSmpGetCpuNum();
    pCpu->IPL = 0xf;
    KeSmpCreateTaskQueue(pCpu);

    printf("Cpu %ld Initialised.\n", pSmpInfo->lapic_id);
    g_SmpStartedCount++;
    SpinLockRelease(&g_SmpLock);
    for (;;) {
    }
}

void KeSmpInit() {
    struct limine_smp_response *pSmpResponse = SmpRequest.response;
    g_pSmpCpuList = (CpuInfo*)MmVirtAllocatePages(g_pKernelPageMap,
        DIV_ROUND_UP(sizeof(CpuInfo) * pSmpResponse->cpu_count, PAGE_SIZE), MM_READ | MM_WRITE);
    memset(g_pSmpCpuList, 0, sizeof(CpuInfo) * pSmpResponse->cpu_count);
    uint64_t BspID = pSmpResponse->bsp_lapic_id;

    CpuInfo *pBspCpu = &g_pSmpCpuList[0];
    KeArchSmpInit(&pBspCpu->ArchInfo);
    pBspCpu->pCurrentPageMap = g_pKernelPageMap;
    pBspCpu->IPL = 0xf;

    MmAllocInit();

    KeSmpCreateTaskQueue(pBspCpu);

    for (uint64_t i = 0; i < pSmpResponse->cpu_count; i++) {
        if (pSmpResponse->cpus[i]->lapic_id != BspID) {
            pSmpResponse->cpus[i]->extra_argument = (uint64_t)&g_pSmpCpuList[i];
            pSmpResponse->cpus[i]->goto_address = (void*)KeSmpCpuInit;
        }
    }
    while (g_SmpStartedCount < pSmpResponse->cpu_count - 1)
        __asm__ volatile ("pause");
    g_SmpStarted = true;
}

CpuInfo *KeSmpGetCpu() {
    return &g_pSmpCpuList[KeArchSmpGetCpuNum()];
}

CpuInfo *KeSmpGetCpuByNum(uint32_t Num) {
    return &g_pSmpCpuList[Num];
}
