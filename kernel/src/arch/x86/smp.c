#include <x86/gdt.h>
#include <x86/idt.h>
#include <x86/smp.h>
#include <x86/apic.h>
#include <limine.h>
#include <alloc.h>
#include <spinlock.h>
#include <printf.h>

void KeArchSmpCpuInit(struct limine_smp_info *pSmpInfo, ArchCpuInfo *pCpuInfo) {
    KeGdtInit();
    KeIdtReload();

    KeLocalApicInit();
    __asm__ volatile ("sti");
    pCpuInfo->LocalApicTicks = KeLocalApicInitTimer();
    pCpuInfo->LocalApicID = pSmpInfo->lapic_id;
}

void KeArchSmpInit(ArchCpuInfo *pCpuInfo) {
    pCpuInfo->LocalApicTicks = KeLocalApicInitTimer();
    pCpuInfo->LocalApicID = 0;
}

uint64_t KeArchSmpGetCpuNum() {
    return KeLocalApicGetID();
}
