#include <x86/idt.h>
#include <x86/apic.h>
#include <x86/ports.h>
#include <context.h>
#include <printf.h>

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
void *g_pIrqHandlers[256];

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

uint8_t KeGetFreeVector() {
    for (int i = 16; i < 256; i++) {
        if (g_pIrqHandlers[i] == NULL)
            return i;
    }
    return 0;
}

void KeInstallIrq(uint8_t Vector, void *Handler, bool Remap) {
    g_pIrqHandlers[Vector] = Handler;
    if (Remap)
        KeIoApicRemapIrq(g_pMpIoApic, Vector, Vector + 32, false);
}

void KeHandleIsr(Context *pContext) {
    printf("ISR Caught: %s.\n", g_pszMessages[pContext->IntNo]);
    for (;;) {
        __asm__ volatile ("hlt;cli");
    }
}

void KeHandleIrq(Context *pContext) {
    void(*fnHandler)(Context*) = g_pIrqHandlers[pContext->IntNo - 32];
    if (!fnHandler) {
        printf("Warning: Caught unhandled IRQ %d\n", pContext->IntNo);
        KeLocalApicEoi();
        return;
    }
    fnHandler(pContext);
}
