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

#define FREE_BIT 1
#define BLK_GET_SIZE(Block) (Block->Size >> 1)

Region *MmCreateRegion(AllocatorDescriptor *pAllocator, size_t AreaSize) {
    Region *pRegion = (Region*)MmVirtAllocatePages(pAllocator->pPageMap, 1, MM_READ | MM_WRITE);
    pRegion->FreeSize = AreaSize - sizeof(Block);
    pRegion->TotalSize = AreaSize;
    pRegion->pDataArea = (uint8_t*)MmVirtAllocatePages(pAllocator->pPageMap, AreaSize / PAGE_SIZE, MM_READ | MM_WRITE);
    pRegion->pNext = NULL;

    Block *pBlock = (Block*)pRegion->pDataArea;
    pBlock->pNext = NULL;
    pBlock->pPrev = NULL;
    pBlock->Size = ((AreaSize - sizeof(Block)) << 1) | FREE_BIT;
    return pRegion;
}

AllocatorDescriptor *MmAllocInit() {
    AllocatorDescriptor *pAllocator = (AllocatorDescriptor*)MmVirtAllocatePages(g_pKernelPageMap, 1, MM_READ | MM_WRITE);
    pAllocator->pPageMap = MmGetPageMap();
    pAllocator->pMainRegion = MmCreateRegion(pAllocator, PAGE_SIZE * 3);
    return pAllocator;
}

Block *MmSplit(Block *pBlock, size_t Size) {
    Block *pNewBlock = (Block*)((uint8_t*)pBlock + sizeof(Block) + Size);
    size_t OldSize = BLK_GET_SIZE(pBlock);
    pNewBlock->pNext = pBlock->pNext;
    pNewBlock->pPrev = pBlock;
    pNewBlock->Size = (OldSize - Size - sizeof(Block)) << 1;
    pBlock->pNext = pNewBlock;
    pBlock->Size = Size << 1;
    return pBlock;
}

void *MmInternalAlloc(AllocatorDescriptor *pAllocator, size_t Size) {
    Region *pRegion;
    bool Found = false;
    for (pRegion = pAllocator->pMainRegion; pRegion->pNext != NULL; pRegion = pRegion->pNext)
        if (pRegion->FreeSize >= Size + sizeof(Block)) {
            Found = true;
            break;
        }
    if (!Found) {
        Region *pNewRegion = MmCreateRegion(pAllocator, ALIGN_UP(Size, PAGE_SIZE) * 2);
        pRegion->pNext = pNewRegion;
        pRegion = pNewRegion;
    }
    Block *pBlock = (Block*)pRegion->pDataArea;
    while (pBlock && !(pBlock->Size & FREE_BIT)) {
        if (BLK_GET_SIZE(pBlock) >= Size && (pBlock->Size & FREE_BIT))
            break;
        pBlock = pBlock->pNext;
    }
    ASSERT(pBlock);
    if (BLK_GET_SIZE(pBlock) > Size) {
        pBlock = MmSplit(pBlock, Size);
        pBlock->pNext->Size |= FREE_BIT;
        pRegion->FreeSize -= sizeof(Block);
    }
    pRegion->FreeSize -= Size;
    pBlock->pRegion = (uint8_t*)pRegion;
    pBlock->Size &= ~FREE_BIT;
    return (void*)((uint8_t*)pBlock + sizeof(Block));
}

void *MmAlloc(size_t Size) {
    if (Size % 0x10) {
        // Align size to 16 so all pointers stay aligned.
        Size -= (Size % 0x10);
        Size += 0x10;
    }
    return MmInternalAlloc(KeSmpGetCpu()->pCurrentAllocator, Size);
}

Block *MmMerge(Block *pBlock, Region *pRegion) {
    size_t NewSize = (BLK_GET_SIZE(pBlock) + BLK_GET_SIZE(pBlock->pNext) + sizeof(Block));
    size_t FreedSize = BLK_GET_SIZE(pBlock) + sizeof(Block);
    pBlock->Size = NewSize << 1;
    pBlock->pNext = pBlock->pNext->pNext;
    if (pBlock->pNext) pBlock->pNext->pPrev = pBlock;
    memset((uint8_t*)pBlock + sizeof(Block), 0, NewSize);
    pRegion->FreeSize += FreedSize;
    return pBlock;
}

void MmFree(void *pPtr) {
    Block *pBlock = (Block*)((uint8_t*)pPtr - sizeof(Block));
    Region *pRegion = (Region*)pBlock->pRegion;
    // Find and merge nearby blocks.
    while (pBlock->pNext && pBlock->pNext->Size & FREE_BIT)
        MmMerge(pBlock, pRegion);
    while (pBlock->pPrev && pBlock->pPrev->Size & FREE_BIT)
        pBlock = MmMerge(pBlock->pPrev, pRegion);
    pBlock->Size |= FREE_BIT;
}

void *MmKAlloc(size_t Size) {
    if (Size % 0x10) {
        // Align size to 16 so all pointers stay aligned.
        Size -= (Size % 0x10);
        Size += 0x10;
    }
    return MmInternalAlloc(g_pKernelAllocator, Size);
}

void MmKFree(void *pPtr) {
    PageMap *pOldPageMap = MmSwitchPageMap(g_pKernelPageMap);
    MmFree(pPtr);
    MmSwitchPageMap(pOldPageMap);
}

void MmDestroy(AllocatorDescriptor *pAllocator) {
    Region *pNextRegion = NULL;
    for (Region *pRegion = pAllocator->pMainRegion; pRegion != NULL; pRegion = pNextRegion) {
        pNextRegion = pRegion->pNext;
        MmVirtFreePages(pAllocator->pPageMap, pRegion->pDataArea);
        MmVirtFreePages(pAllocator->pPageMap, pRegion);
    }
}
