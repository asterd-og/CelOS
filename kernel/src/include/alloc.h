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
    size_t FreeSize;
    size_t TotalSize;
    uint8_t *pDataArea;
} Region;

typedef struct {
    PageMap *pPageMap;
    Region *pMainRegion;
} AllocatorDescriptor;

AllocatorDescriptor *MmAllocInit();
void *MmAlloc(size_t size);
void MmFree(void *ptr);
void MmAllocPrint(AllocatorDescriptor *pAllocator);
void MmAllocDestroy(AllocatorDescriptor *pAllocator);