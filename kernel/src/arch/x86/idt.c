#include <x86/idt.h>
#include <x86/apic.h>
#include <x86/ports.h>
#include <x86/e9.h>
#include <context.h>
#include <printf.h>
#include <interrupt.h>
#include <vmm.h>
#include <pmm.h>
#include <sched.h>

const char *g_pszMessages[32] = {
    "Division by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Detected overflow",
    "Out-of-bounds",
    "Invalid opcode",
    "No coprocessor",
    "Double fault",
    "Coprocessor segment overrun",
    "Bad TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Unknown interrupt",
    "Coprocessor fault",
    "Alignment check",
    "Machine check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

__attribute__((aligned(0x10))) static IdtEntry g_IdtEntries[256];
static IdtDescriptor g_IdtDesc;
extern void *g_pIdtIntTable[];

void KeSetIdtEntry(uint16_t Vector, void *Isr, uint8_t Flags);

void KeIdtInit() {
    for (uint16_t Vector = 0; Vector < 256; Vector++) {
        KeSetIdtEntry(Vector, g_pIdtIntTable[Vector], 0x8E);
    }

    g_IdtDesc.Size = sizeof(IdtEntry) * 256 - 1;
    g_IdtDesc.Offset = (uint64_t)g_IdtEntries;

    __asm__ volatile ("lidt %0" : : "m"(g_IdtDesc) : "memory");
}

void KeIdtReload() {
    __asm__ volatile ("lidt %0" : : "m"(g_IdtDesc) : "memory");
}

void KeSetIdtEntry(uint16_t Vector, void *Isr, uint8_t Flags) {
    IdtEntry *pEntry = &g_IdtEntries[Vector];
    pEntry->OffsetLow = (uint64_t)Isr & 0xFFFF;
    pEntry->Selector = 0x08; // 64 Bit CS
    pEntry->Ist = 0;
    pEntry->Flags = Flags;
    pEntry->OffsetMid = ((uint64_t)Isr >> 16) & 0xFFFF;
    pEntry->OffsetHigh = ((uint64_t)Isr >> 32) & 0xFFFFFFFF;
    pEntry->Reserved = 0;
}

void KeHandleIsr(Context *pContext) {
    KxBlockSched();
    printf("ISR Caught: %s @ 0x%lx.\n", g_pszMessages[pContext->IntNo], pContext->RIP);
    E9Write("ISR Caught: %s @ 0x%lx.\n", g_pszMessages[pContext->IntNo], pContext->RIP);
    if (pContext->IntNo == 14) {
        __asm__ volatile ("cli");
        uint64_t CR2 = 0;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(CR2) :);
        //CR2 = ALIGN_DOWN(CR2, PAGE_SIZE);
        MmPageFault(CR2);
    }
    for (;;) {
        __asm__ volatile ("hlt\ncli");
    }
}

void KeHandleIrq(Context *pContext) {
    int Ret = KxHandleIrq(pContext->IntNo - 32, pContext);
    if (Ret == INT_UNCAUGHT) {
        printf("BAD: Uncaught interrupt %d.\n", pContext->IntNo);
        return;
    } else if (Ret == INT_QUEUED) {
        printf("INFO: Queued interrupt %d.\n", pContext->IntNo);
        return;
    }
}
