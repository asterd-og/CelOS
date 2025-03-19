/*

Memory allocator written by: Astrido

*/

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <vmm.h>

#define HEAP_SIG 0xCACEFACEC001ABAB

typedef struct HeapPool {
    struct HeapPool *pNext;
    uint8_t *pDataArea;
    size_t FreeSize;
    size_t TotalSize;
} HeapPool;

typedef struct HeapBlock {
    uint64_t Signature;
    HeapPool *pPool; // Address of the region it's located in
    struct HeapBlock *pNext;
    struct HeapBlock *pPrev;
    size_t Size;
    uint8_t Status; // 0 for free 1 for used
} HeapBlock;

typedef struct {
    HeapPool *pMainPool;
} HeapCtrl;

void MmAllocInit();
void *MmAlloc(size_t Size);
void *MmRealloc(void *pPtr, size_t Size);
void MmFree(void *pPtr);