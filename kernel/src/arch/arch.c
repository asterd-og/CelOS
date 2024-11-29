#include <arch.h>

#if defined (__x86_64__)
#include <x86/gdt.h>
#include <x86/idt.h>
#include <x86/acpi.h>
#include <x86/madt.h>
#include <x86/apic.h>
#include <x86/ports.h>
#include <x86/pit.h>
#elif defined (__riscv)
#endif
#include <pmm.h>
#include <vmm.h>
#include <alloc.h>
#include <smp.h>

void KeArchInit() {
#if defined (__x86_64__)
    __asm__ volatile ("cli");
    KeGdtInit();
    KeIdtInit();
#endif
    MmPhysInit();
#if defined (__x86_64__)
    MmVirtInit();
    MmAllocInit();
    KeAcpiInit();
    KeMadtInit();
    KeLocalApicInit();
    KeIoApicInit();
    __asm__ volatile ("sti");
    KePitInit();
#endif
    KeSmpInit();
}
