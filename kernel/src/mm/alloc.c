/*

Memory allocator written by: Astrido

*/

#include <alloc.h>
#include <pmm.h>
#include <smp.h>
#include <stdbool.h>
#include <string.h>
#include <printf.h>
#include <assert.h>

HeapCtrl *pKernelHeap = NULL;

HeapPool *MmAllocPool(size_t Size) {
    HeapPool *pPool = MmVirtAllocatePages(g_pKernelPageMap, 1, MM_READ | MM_WRITE);
    pPool->pNext = NULL;
    pPool->pDataArea = MmVirtAllocatePages(g_pKernelPageMap, DIV_ROUND_UP(Size, PAGE_SIZE), MM_READ | MM_WRITE);
    memset(pPool->pDataArea, 0, Size);
    pPool->FreeSize = Size - sizeof(HeapBlock);
    pPool->TotalSize = Size;
    HeapBlock *pBlock = (HeapBlock*)pPool->pDataArea;
    pBlock->Signature = HEAP_SIG;
    pBlock->pPool = pPool;
    pBlock->Size = Size - sizeof(HeapBlock);
    return pPool;
}

void MmAllocInit() {
    pKernelHeap = MmVirtAllocatePages(g_pKernelPageMap, 1, MM_READ | MM_WRITE);
    pKernelHeap->pMainPool = MmAllocPool(PAGE_SIZE * 10);
}

void MmAllocSplitBlock(HeapBlock *pBlock, size_t TotalSize, size_t DesiredSize) {
    HeapBlock *pNewBlock = (HeapBlock*)(((uint8_t*)pBlock) + sizeof(HeapBlock) + DesiredSize);
    pNewBlock->Signature = HEAP_SIG;
    pNewBlock->pPool = pBlock->pPool;
    pNewBlock->pNext = pBlock->pNext;
    pNewBlock->pPrev = pBlock;
    pNewBlock->Size = (TotalSize - sizeof(HeapBlock)) - DesiredSize;
    pNewBlock->Status = 0;
    pBlock->pNext = pNewBlock;
    pBlock->Size = DesiredSize;
    pBlock->Status = 0;
}

void *MmAlloc(size_t Size) {
    // Find appropriate pool
    HeapPool *pPool = pKernelHeap->pMainPool;
    while (pPool->FreeSize < Size + sizeof(HeapBlock)) {
        if (pPool->pNext == NULL)
            return NULL;
        pPool = pPool->pNext;
    }

    // Find appropriate block
    HeapBlock *pBlock = (HeapBlock*)pPool->pDataArea;
    while (pBlock->Status == 1 || pBlock->Size < Size) {
        if (pBlock->pNext == NULL)
            return NULL;
        pBlock = pBlock->pNext;
    }

    // Split if necessary
    if (pBlock->Size > Size) {
        MmAllocSplitBlock(pBlock, pBlock->Size, Size);
        pPool->FreeSize -= sizeof(HeapBlock);
    }

    pBlock->Signature = HEAP_SIG;
    pBlock->pPool = pPool;
    pBlock->Status = 1;

    pPool->FreeSize -= Size;
    return (void*)(((uint8_t*)pBlock) + sizeof(HeapBlock));
}

void MmMerge(HeapBlock *pBlock, HeapPool *pPool) {
    size_t NewSize = pBlock->Size + pBlock->pNext->Size + sizeof(HeapBlock);
    size_t FreedSize = pBlock->Size + sizeof(HeapBlock);
    pBlock->Size = NewSize;
    if (pBlock->pNext->pNext) {
        pBlock->pNext = pBlock->pNext->pNext;
        pBlock->pNext->pPrev = pBlock;
    } else
        pBlock->pNext = NULL;
    memset((uint8_t*)pBlock + sizeof(HeapBlock), 0, NewSize);
    pPool->FreeSize += FreedSize;
}

void MmFree(void *pPtr) {
    HeapBlock *pBlock = (HeapBlock*)(((uint8_t*)pPtr) - sizeof(HeapBlock));
    ASSERT(pBlock->Signature == HEAP_SIG);
    HeapPool *pPool = pBlock->pPool;

    while (pBlock->pNext && pBlock->pNext->Status == 0)
        MmMerge(pBlock, pPool);

    while (pBlock->pPrev && pBlock->pPrev->Status == 0) {
        pBlock = pBlock->pPrev;
        MmMerge(pBlock, pPool);
    }

    pBlock->Status = 0;
}