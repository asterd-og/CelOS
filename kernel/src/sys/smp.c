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

AllocatorDescriptor *g_pKernelAllocator = NULL;

bool g_SmpStarted = false;

void KeSmpCreateThreadQueue(CpuInfo *pCpu) {
    ThreadQueue *pThreadHighQueue = (ThreadQueue*)MmAlloc(sizeof(ThreadQueue));
    ThreadQueue *pThreadMedQueue = (ThreadQueue*)MmAlloc(sizeof(ThreadQueue));
    ThreadQueue *pThreadLowQueue = (ThreadQueue*)MmAlloc(sizeof(ThreadQueue));
    pThreadHighQueue->pThreads = NULL;
    pThreadHighQueue->Priority = THREAD_HIGH;
    pThreadHighQueue->pNext = pThreadMedQueue;
    pThreadHighQueue->HasRunnableThread = false;

    pThreadMedQueue->pThreads = NULL;
    pThreadMedQueue->Priority = THREAD_MED;
    pThreadMedQueue->pNext = pThreadLowQueue;
    pThreadMedQueue->HasRunnableThread = false;

    pThreadLowQueue->pThreads = NULL;
    pThreadLowQueue->Priority = THREAD_LOW;
    pThreadLowQueue->pNext = NULL;
    pThreadLowQueue->HasRunnableThread = false;

    pCpu->pThreadQueue = pThreadHighQueue;
}

void KeSmpCpuInit(struct limine_smp_info *pSmpInfo) {
    SpinLockAcquire(&g_SmpLock);
    CpuInfo *pCpu = (CpuInfo*)pSmpInfo->extra_argument;
    MmSwitchPageMap(g_pKernelPageMap);
    pCpu->pCurrentPageMap = g_pKernelPageMap;

    KeArchSmpCpuInit(pSmpInfo, &pCpu->ArchInfo);
    pCpu->CpuNum = KeArchSmpGetCpuNum();
    pCpu->IPL = 0xf;
    pCpu->pCurrentAllocator = g_pKernelAllocator;
    KeSmpCreateThreadQueue(pCpu);

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

    g_pKernelAllocator = MmAllocInit();
    pBspCpu->pCurrentAllocator = g_pKernelAllocator;

    KeSmpCreateThreadQueue(pBspCpu);

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

AllocatorDescriptor *KeSmpSwitchAllocator(AllocatorDescriptor *pAllocator) {
    CpuInfo *pCpu = KeSmpGetCpu();
    AllocatorDescriptor *pOldAllocator = pCpu->pCurrentAllocator;
    pCpu->pCurrentAllocator = pAllocator;
    return pOldAllocator;
}
