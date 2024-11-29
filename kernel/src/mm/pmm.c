#include <pmm.h>
#include <limine.h>
#include <string.h>
#include <panic.h>

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request MmapRequest = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

uint64_t *pAddressStack;
uint64_t iAddressStackIndex = 0;
uint64_t cxAddressStackCount = 0;

void MmPhysStackPush(uint64_t Data) {
    if (iAddressStackIndex + 1 > cxAddressStackCount) {
        PANIC("Pushing more addresses than maximum capacity.");
        return;
    }
    pAddressStack[iAddressStackIndex++] = Data;
}

uint64_t MmPhysStackPop() {
    if (iAddressStackIndex == 0)
        return 0;
    iAddressStackIndex--;
    return pAddressStack[iAddressStackIndex];
}

struct limine_memmap_response *g_pMmapResponse = NULL;

void MmPhysInit() {
    g_pMmapResponse = MmapRequest.response;
    struct limine_memmap_entry **MmapEntries = g_pMmapResponse->entries;
    struct limine_memmap_entry *MmapEntry;

    uint64_t cbStackSize = 0;
    for (uint64_t i = 0; i < g_pMmapResponse->entry_count; i++) {
        MmapEntry = MmapEntries[i];
        if (MmapEntry->type == LIMINE_MEMMAP_USABLE)
            cxAddressStackCount += DIV_ROUND_UP(MmapEntry->length, PAGE_SIZE);
    }
    cbStackSize = ALIGN_UP(cxAddressStackCount * 8, PAGE_SIZE);
    for (uint64_t i = 0; i < g_pMmapResponse->entry_count; i++) {
        MmapEntry = MmapEntries[i];
        if (MmapEntry->type != LIMINE_MEMMAP_USABLE || MmapEntry->length < cbStackSize) continue;
        pAddressStack = (uint64_t*)HIGHER_HALF(MmapEntry->base);
        memset(pAddressStack, 0, cbStackSize);
        MmapEntry->base += cbStackSize;
        MmapEntry->length -= cbStackSize;
        break;
    }
    for (uint64_t i = 0; i < g_pMmapResponse->entry_count; i++) {
        MmapEntry = MmapEntries[i];
        if (MmapEntry->type == LIMINE_MEMMAP_USABLE)
            for (uint64_t j = 0; j < MmapEntry->length; j += PAGE_SIZE)
                MmPhysStackPush(MmapEntry->base + j);
    }
}

void *MmPhysAllocatePage() {
    uint64_t Address = MmPhysStackPop();
    return (void*)Address;
}

void MmPhysFreePage(void *pAddress) {
    MmPhysStackPush((uint64_t)pAddress);
}
