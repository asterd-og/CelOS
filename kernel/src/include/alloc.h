/*

Memory allocator written by: Astrido

*/

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <vmm.h>

typedef struct Block {
    uint8_t *pRegion; // Address of the region it's located in
    struct Block *pNext;
    struct Block *pPrev;
    size_t Size;
} Block;

typedef struct Region {
    struct Region *pNext;
    uint8_t *pDataArea;
    size_t FreeSize;
    size_t TotalSize;
} Region;

typedef struct {
    PageMap *pPageMap;
    Region *pMainRegion;
} AllocatorDescriptor;

AllocatorDescriptor *MmAllocInit();
void *MmAlloc(size_t Size);
void MmFree(void *pPtr);
void *MmKAlloc(size_t Size);
void MmKFree(void *pPtr);
void MmAllocPrint(AllocatorDescriptor *pAllocator);
void MmAllocDestroy(AllocatorDescriptor *pAllocator);