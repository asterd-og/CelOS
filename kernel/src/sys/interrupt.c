#include <interrupt.h>
#include <smp.h>
#if defined (__x86_64__)
#include <x86/idt.h>
#include <x86/apic.h>
#endif
#include <panic.h>
#include <printf.h>

IrqDescriptor g_IrqTbl[224];

int KxHandleIrq(uint8_t Irq, Context *pCtx) {
    if (!g_IrqTbl[Irq].Present)
        return INT_UNCAUGHT;
    CpuInfo *pCpu = KeSmpGetCpu();
    if (pCpu->RunningIrq) {
        if (pCpu->IPL < g_IrqTbl[Irq].IPL) {
            // Enqueue this IRQ since it has an IPL higher than the current one.
            if (pCpu->QueuedIrqIdx == 255) {
                PANIC("Too many waiting IRQs.");
                return INT_UNCAUGHT;
            }
            pCpu->QueuedIrqs[pCpu->QueuedIrqIdx++] = Irq;
            return INT_QUEUED;
        }
    }
    g_IrqTbl[Irq].pHandler(pCtx);
    for (uint8_t i = 0; i < pCpu->QueuedIrqIdx; i++) {
        Irq = pCpu->QueuedIrqs[i];
        pCpu->IPL = g_IrqTbl[Irq].IPL;
        KxSendInt(pCpu->CpuNum, Irq); // TODO: Test this
    }
    return INT_OK;
}

uint8_t KxGetFreeIrq() {
    for (int Irq = 16; Irq < 224; Irq++)
        if (!g_IrqTbl[Irq].Present)
            return Irq;
    return 0;
}

void KxInstallIrq(uint8_t Irq, void *pHandler, uint8_t IPL) {
    g_IrqTbl[Irq].pHandler = pHandler;
    g_IrqTbl[Irq].IPL = IPL;
    g_IrqTbl[Irq].Present = true;
    #if defined (__x86_64__)
    if (Irq < 16)
        KeIoApicRemapIrq(g_pMpIoApic, Irq, Irq + 32, false);
    #endif
}

void KxUninstallIrq(uint8_t Irq) {
    g_IrqTbl[Irq].Present = false;
    #if defined (__x86_64__)
    if (Irq < 16)
        KeIoApicRemapIrq(g_pMpIoApic, Irq, Irq + 32, true);
    #endif
}

void KxSendInt(uint32_t CpuNum, uint8_t Irq) {
    #if defined (__x86_64__)
    KeLocalApicIpi(CpuNum, Irq + 32);
    #endif
}

void KxLowerIpl(uint8_t IPL) {
}