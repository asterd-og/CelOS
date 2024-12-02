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
#include <printf.h>

void KeArchInit() {
#if defined (__x86_64__)
    __asm__ volatile ("cli");
    KeGdtInit();
    KeIdtInit();
    printf("OK: Idt Initialized.\n");
#endif
    MmPhysInit();
    printf("OK: Physical MM Initialized.\n");
    MmVirtInit();
    printf("OK: Virtual MM Initialized.\n");
    printf("OK: MM Initialized.\n");
#if defined (__x86_64__)
    KeAcpiInit();
    printf("OK: ACPI Initialized.\n");
    KeMadtInit();
    KeLocalApicInit();
    KeIoApicInit();
    printf("OK: APIC Initialized.\n");
    __asm__ volatile ("sti");
    KePitInit();
#endif
    KeSmpInit();
    printf("OK: SMP Initialized.\n");
#if defined (__x86_64__)
    KePitUninstall();
#endif
}
