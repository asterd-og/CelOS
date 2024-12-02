#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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
    bool Free;
    uint64_t VirtualAddress;
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
PageMap *MmSwitchPageMap(PageMap *pPageMap);
PageMap *MmNewPageMap();
uint64_t MmVirtMap(PageMap *pPageMap, uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Flags);
void MmVirtUnmap(PageMap *pPageMap, uint64_t VirtualAddress);
uint64_t MmGetPagePhysicalAddress(PageMap *pPageMap, uint64_t VirtualAddress);
void *MmVirtAllocatePages(PageMap *pPageMap, uint64_t Pages, uint64_t Flags);
void MmVirtFreePages(PageMap *pPageMap, void *pPtr);
PageMap *MmGetPageMap();
