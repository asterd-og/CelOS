#include <mmu.h>
#include <pmm.h>
#include <vmm.h>
#include <limine.h>
#include <string.h>
#include <panic.h>
#include <printf.h>
#include <assert.h>
#include <stdbool.h>

#define MM_PTE_ADDR_MASK 0x000ffffffffff000
#define MM_PTE_GET_ADDR(x) ((x) & MM_PTE_ADDR_MASK)
#define MM_PTE_GET_FLAGS(x) ((x) & ~MM_PTE_ADDR_MASK)

void MmArchInvalidatePage(uint64_t VirtualAddress);

void MmArchSwitchPageMap(uint64_t *pPageMap) {
    __asm__ volatile ("mov %0, %%cr3" : : "r"((uint64_t)PHYSICAL(pPageMap)) : "memory");
}

uint64_t *MmArchGetNextLevel(uint64_t *pLevel, uint64_t Entry, bool Allocate) {
    if (pLevel[Entry] & MM_READ)
        return HIGHER_HALF(MM_PTE_GET_ADDR(pLevel[Entry]));
    else if (!Allocate)
        return NULL;
    uint64_t *pPageMapLevel = HIGHER_HALF(MmPhysAllocatePage());
    ASSERT(PHYSICAL(pPageMapLevel));
    memset(pPageMapLevel, 0, PAGE_SIZE);
    pLevel[Entry] = (uint64_t)PHYSICAL(pPageMapLevel) | MM_READ | MM_WRITE;
    return pPageMapLevel;
}

uint64_t *MmArchTraverseLevels(uint64_t *pPageMap, uint64_t VirtualAddress, bool Allocate) {
    uint64_t PageMapLevel2Entry = (VirtualAddress >> 21) & 0x1ff;
    uint64_t PageMapLevel3Entry = (VirtualAddress >> 30) & 0x1ff;
    uint64_t PageMapLevel4Entry = (VirtualAddress >> 39) & 0x1ff;

    uint64_t *pPageMapLevel3 = MmArchGetNextLevel(pPageMap, PageMapLevel4Entry, Allocate);
    if (!pPageMapLevel3) return NULL;
    uint64_t *pPageMapLevel2 = MmArchGetNextLevel(pPageMapLevel3, PageMapLevel3Entry, Allocate);
    if (!pPageMapLevel2) return NULL;
    uint64_t *pPageMapLevel1 = MmArchGetNextLevel(pPageMapLevel2, PageMapLevel2Entry, Allocate);
    if (!pPageMapLevel1) return NULL;

    return pPageMapLevel1;
}

void MmArchVirtMap(uint64_t *pPageMap, uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Flags) {
    uint64_t PageMapLevel1Entry = (VirtualAddress >> 12) & 0x1ff;
    uint64_t *pPageMapLevel1 = MmArchTraverseLevels(pPageMap, VirtualAddress, true);

    pPageMapLevel1[PageMapLevel1Entry] = PhysicalAddress | Flags;
}

void MmArchVirtUnmap(uint64_t *pPageMap, uint64_t VirtualAddress) {
    uint64_t PageMapLevel1Entry = (VirtualAddress >> 12) & 0x1ff;
    uint64_t *pPageMapLevel1 = MmArchTraverseLevels(pPageMap, VirtualAddress, false);
    if (!pPageMapLevel1) return;

    pPageMapLevel1[PageMapLevel1Entry] = 0;
    MmArchInvalidatePage(VirtualAddress);
}

uint64_t MmArchGetPagePhysicalAddress(uint64_t *pPageMap, uint64_t VirtualAddress) {
    uint64_t PageMapLevel1Entry = (VirtualAddress >> 12) & 0x1ff;
    uint64_t *pPageMapLevel1 = MmArchTraverseLevels(pPageMap, VirtualAddress, false);
    if (!pPageMapLevel1) return 0;

    return pPageMapLevel1[PageMapLevel1Entry] & ~0xfff;
}
