#pragma once

#include <stdint.h>
#include <stddef.h>

#if defined (__x86_64__)
#define MM_READ 1
#define MM_WRITE 2
#define MM_USER 4
#define MM_NX 1ul << 63
#elif defined (__riscv)
#define MM_READ 1
#define MM_WRITE 2
#define MM_USER 4
#define MM_NX 8
#endif

typedef struct VirtMemRegion {
    uint64_t VirtualAddress;
    uint64_t PhysicalAddress;
    uint64_t Pages;
    uint64_t Flags;
    struct VirtMemRegion* pNext;
    struct VirtMemRegion* pPrev;
} VirtMemRegion;

typedef struct {
    uint64_t *pTopLevel;
    VirtMemRegion *pVirtMemHead;
} PageMap;

extern PageMap *g_pKernelPageMap;

void MmVirtInit();
void MmSwitchPageMap(PageMap *pPageMap);
PageMap *MmNewPageMap();
void MmVirtMap(PageMap *pPageMap, uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Flags);
void MmVirtUnmap(PageMap *pPageMap, uint64_t VirtualAddress);
uint64_t MmGetPagePhysicalAddress(PageMap *pPageMap, uint64_t VirtualAddress);
void *MmVirtAllocatePages(PageMap *pPageMap, uint64_t Pages, uint64_t Flags);
void MmVirtFreePages(PageMap *pPageMap, void *pPtr);