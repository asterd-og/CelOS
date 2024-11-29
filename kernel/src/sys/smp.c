#include <smp.h>
#include <limine.h>
#include <alloc.h>
#include <spinlock.h>
#include <printf.h>

__attribute__((used, section(".limine_requests")))
static volatile struct limine_smp_request SmpRequest = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0
};

CpuInfo *g_pSmpCpuList;
SpinLock g_SmpLock;
uint64_t g_SmpStartedCount = 0;

void KeSmpCpuInit(struct limine_smp_info *pSmpInfo) {
    SpinLockAcquire(&g_SmpLock);
    CpuInfo *pCpu = (CpuInfo*)pSmpInfo->extra_argument;
    MmSwitchPageMap(g_pKernelPageMap);
    pCpu->pCurrentPageMap = g_pKernelPageMap;

    KeArchSmpCpuInit(pSmpInfo, &pCpu->ArchInfo);
    pCpu->CpuNum = KeArchSmpGetCpuNum();
    pCpu->pProcList = NULL;
    pCpu->pCurrentThread = NULL;
    pCpu->pCurrentProc = NULL;

    printf("Cpu %ld Initialised.\n", pSmpInfo->lapic_id);
    g_SmpStartedCount++;
    SpinLockRelease(&g_SmpLock);
    for (;;) {
    }
}

void KeSmpInit() {
    struct limine_smp_response *pSmpResponse = SmpRequest.response;
    g_pSmpCpuList = (CpuInfo*)MmAlloc(sizeof(CpuInfo) * pSmpResponse->cpu_count);
    uint64_t BspID = pSmpResponse->bsp_lapic_id;

    CpuInfo *BspCpu = &g_pSmpCpuList[0];
    KeArchSmpInit(&BspCpu->ArchInfo);
    BspCpu->pCurrentPageMap = g_pKernelPageMap;
    BspCpu->CpuNum = 0;
    BspCpu->pProcList = NULL;
    BspCpu->pCurrentThread = NULL;
    BspCpu->pCurrentProc = NULL;

    for (uint64_t i = 0; i < pSmpResponse->cpu_count; i++) {
        if (pSmpResponse->cpus[i]->lapic_id != BspID) {
            pSmpResponse->cpus[i]->extra_argument = (uint64_t)&g_pSmpCpuList[i];
            pSmpResponse->cpus[i]->goto_address = (void*)KeSmpCpuInit;
        }
    }
    while (g_SmpStartedCount < pSmpResponse->cpu_count - 1)
        __asm__ volatile ("pause");
}

CpuInfo *KeSmpGetCpu() {
    return &g_pSmpCpuList[KeArchSmpGetCpuNum()];
}

CpuInfo *KeSmpGetCpuByNum(uint32_t Num) {
    return &g_pSmpCpuList[Num];
}
