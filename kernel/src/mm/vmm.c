#include <vmm.h>
#include <pmm.h>
#include <limine.h>
#include <string.h>
#include <panic.h>
#include <printf.h>
#include <assert.h>
#include <stdbool.h>
#include <mmu.h>

__attribute__((used, section(".limine_requests")))
static volatile struct limine_kernel_address_request KernelAddressRequest = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

PageMap *g_pKernelPageMap;

extern char LimineRequestsStartLD[];
extern char LimineRequestsEndLD[];

extern char TextStartLD[];
extern char TextEndLD[];

extern char RodataStartLD[];
extern char RodataEndLD[];

extern char DataStartLD[];
extern char DataEndLD[];

VirtMemRegion *MmNewRegion(uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Pages, uint64_t Flags) {
    VirtMemRegion *pRegion = HIGHER_HALF(MmPhysAllocatePage());
    if (!(PHYSICAL(pRegion))) return NULL;
    pRegion->VirtualAddress = VirtualAddress;
    pRegion->PhysicalAddress = PhysicalAddress;
    pRegion->Pages = Pages;
    pRegion->Flags = Flags;
    return pRegion;
}

VirtMemRegion *MmAppendRegion(PageMap *pPageMap, VirtMemRegion *pRegion) {
    pRegion->pNext = pPageMap->pVirtMemHead;
    pRegion->pPrev = pPageMap->pVirtMemHead->pPrev;
    pPageMap->pVirtMemHead->pPrev->pNext = pRegion;
    pPageMap->pVirtMemHead->pPrev = pRegion;
    return pRegion;
}

VirtMemRegion *MmInsertRegion(VirtMemRegion *pRegion, VirtMemRegion *pRegionAfter) {
    pRegion->pNext = pRegionAfter->pNext;
    pRegion->pPrev = pRegionAfter;
    pRegionAfter->pNext->pPrev = pRegion;
    pRegionAfter->pNext = pRegion;
    return pRegion;
}

void MmDestroyRegion(VirtMemRegion *pRegion) {
    pRegion->pNext->pPrev = pRegion->pPrev;
    pRegion->pPrev->pNext = pRegion->pNext;
    MmPhysFreePage(pRegion);
}

void MmVirtInit() {
    struct limine_kernel_address_response *KernelAddressResponse = KernelAddressRequest.response;
    uint64_t KernelVirtualAddress = KernelAddressResponse->virtual_base;
    uint64_t KernelPhysicalAddress = KernelAddressResponse->physical_base;
    g_pKernelPageMap = HIGHER_HALF(MmPhysAllocatePage());
    memset(g_pKernelPageMap, 0, PAGE_SIZE);

    g_pKernelPageMap->pTopLevel = HIGHER_HALF(MmPhysAllocatePage());
    memset(g_pKernelPageMap->pTopLevel, 0, PAGE_SIZE);

    g_pKernelPageMap->pVirtMemHead = HIGHER_HALF(MmPhysAllocatePage());
    memset(g_pKernelPageMap->pVirtMemHead, 0, PAGE_SIZE);
    g_pKernelPageMap->pVirtMemHead->pNext = g_pKernelPageMap->pVirtMemHead;
    g_pKernelPageMap->pVirtMemHead->pPrev = g_pKernelPageMap->pVirtMemHead;

    uint64_t LimineRequestsStart = ALIGN_DOWN((uint64_t)LimineRequestsStartLD, PAGE_SIZE);
    uint64_t LimineRequestsEnd = ALIGN_UP((uint64_t)LimineRequestsEndLD, PAGE_SIZE);

    uint64_t TextStart = ALIGN_DOWN((uint64_t)TextStartLD, PAGE_SIZE);
    uint64_t TextEnd = ALIGN_UP((uint64_t)TextEndLD, PAGE_SIZE);

    uint64_t RodataStart = ALIGN_DOWN((uint64_t)RodataStartLD, PAGE_SIZE);
    uint64_t RodataEnd = ALIGN_UP((uint64_t)RodataEndLD, PAGE_SIZE);

    uint64_t DataStart = ALIGN_DOWN((uint64_t)DataStartLD, PAGE_SIZE);
    uint64_t DataEnd = ALIGN_UP((uint64_t)DataEndLD, PAGE_SIZE);

    for (uint64_t gb4 = 0; gb4 < 0x100000000; gb4 += PAGE_SIZE)
        MmVirtMap(g_pKernelPageMap, (uint64_t)HIGHER_HALF(gb4), gb4, MM_READ | MM_WRITE);

    struct limine_memmap_entry *pMmapEntry;
    for (uint64_t i = 0; i < g_pMmapResponse->entry_count; i++) {
        pMmapEntry = g_pMmapResponse->entries[i];
        for (uint64_t j = 0; j < pMmapEntry->length; j += PAGE_SIZE) {
            MmVirtMap(g_pKernelPageMap, (uint64_t)HIGHER_HALF(pMmapEntry->base + j), pMmapEntry->base + j, MM_READ | MM_WRITE | MM_NX);
        }
    }

    for (uint64_t LimineRequests = LimineRequestsStart; LimineRequests < LimineRequestsEnd; LimineRequests += PAGE_SIZE)
        MmVirtMap(g_pKernelPageMap, LimineRequests, LimineRequests - KernelVirtualAddress + KernelPhysicalAddress, MM_READ);

    for (uint64_t Text = TextStart; Text < TextEnd; Text += PAGE_SIZE)
        MmVirtMap(g_pKernelPageMap, Text, Text - KernelVirtualAddress + KernelPhysicalAddress, MM_READ);

    for (uint64_t Rodata = RodataStart; Rodata < RodataEnd; Rodata += PAGE_SIZE)
        MmVirtMap(g_pKernelPageMap, Rodata, Rodata - KernelVirtualAddress + KernelPhysicalAddress, MM_READ | MM_NX);

    for (uint64_t Data = DataStart; Data < DataEnd; Data += PAGE_SIZE)
        MmVirtMap(g_pKernelPageMap, Data, Data - KernelVirtualAddress + KernelPhysicalAddress, MM_READ | MM_WRITE | MM_NX);

    VirtMemRegion *pMemRegion = MmNewRegion(0, 0, 1, MM_READ | MM_WRITE);
    MmAppendRegion(g_pKernelPageMap, pMemRegion);

    MmSwitchPageMap(g_pKernelPageMap);
}

void MmSwitchPageMap(PageMap *pPageMap) {
    MmArchSwitchPageMap(pPageMap->pTopLevel);
}

PageMap *MmNewPageMap() {
    PageMap *pPageMap = (PageMap*)HIGHER_HALF(MmPhysAllocatePage());

    pPageMap->pTopLevel = HIGHER_HALF(MmPhysAllocatePage());
    memset(pPageMap->pTopLevel, 0, PAGE_SIZE);

    pPageMap->pVirtMemHead = HIGHER_HALF(MmPhysAllocatePage());
    memset(pPageMap->pVirtMemHead, 0, PAGE_SIZE);
    pPageMap->pVirtMemHead->pNext = pPageMap->pVirtMemHead;
    pPageMap->pVirtMemHead->pPrev = pPageMap->pVirtMemHead;

    for (uint64_t i = 256; i < 512; i++)
        pPageMap->pTopLevel[i] = g_pKernelPageMap->pTopLevel[i];
    return pPageMap;
}

void MmVirtMap(PageMap *pPageMap, uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Flags) {
    MmArchVirtMap(pPageMap->pTopLevel, VirtualAddress, PhysicalAddress, Flags);
}

void MmVirtUnmap(PageMap *pPageMap, uint64_t VirtualAddress) {
    MmArchVirtUnmap(pPageMap->pTopLevel, VirtualAddress);
}

uint64_t MmGetPagePhysicalAddress(PageMap *pPageMap, uint64_t VirtualAddress) {
    return MmArchGetPagePhysicalAddress(pPageMap->pTopLevel, VirtualAddress);
}

// TODO: Search for first fit
void *MmVirtAllocatePages(PageMap *pPageMap, uint64_t Pages, uint64_t Flags) {
    uint64_t VirtualAddress = pPageMap->pVirtMemHead->pPrev->VirtualAddress + (pPageMap->pVirtMemHead->pPrev->Pages * PAGE_SIZE);
    uint64_t PhysicalAddress = 0;
    for (uint64_t i = 0; i < Pages; i++) {
        PhysicalAddress = (uint64_t)MmPhysAllocatePage();
        MmVirtMap(pPageMap, VirtualAddress + i * PAGE_SIZE, PhysicalAddress + i * PAGE_SIZE, Flags);
    }
    VirtMemRegion *pRegion = MmNewRegion(VirtualAddress, PhysicalAddress, Pages, Flags);
    MmAppendRegion(pPageMap, pRegion);
    return (void*)VirtualAddress;
}

void MmVirtFreePages(PageMap *pPageMap, void *pPtr) {
    uint64_t VirtualAddress = (uint64_t)pPtr;
    if (VirtualAddress & 0xfff) {
        PANIC("Trying to free an invalid virtual address.\n");
        return;
    }
    for (VirtMemRegion *pRegion = pPageMap->pVirtMemHead->pNext; pRegion != pPageMap->pVirtMemHead; pRegion = pRegion->pNext) {
        if (pRegion->VirtualAddress == VirtualAddress) {
            uint64_t PhysicalAddress;
            for (uint64_t i = 0; i < pRegion->Pages; i++) {
                PhysicalAddress = MmGetPagePhysicalAddress(pPageMap, VirtualAddress + (i * PAGE_SIZE));
                MmPhysFreePage((void*)PhysicalAddress);
                MmVirtUnmap(pPageMap, VirtualAddress + (i * PAGE_SIZE));
            }
            MmDestroyRegion(pRegion);
            break;
        }
    }
}
