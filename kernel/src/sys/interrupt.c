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
    if (pCpu->RunningIrq && pCpu->IPL > g_IrqTbl[Irq].IPL) {
        // running IRQ and there's an IPL
        // Enqueue this IRQ since it has an IPL higher than the current one, or another IRQ is running already.
        if (pCpu->QueuedIrqIdx == 255) {
            PANIC("Too many waiting IRQs.");
            return INT_UNCAUGHT;
        }
        printf("INFO: Queueing IRQ %d (Queue index %d).\n", Irq, pCpu->QueuedIrqIdx); // Debugging purposes.
        pCpu->QueuedIrqs[pCpu->QueuedIrqIdx++] = Irq;
        return INT_QUEUED;
    }
    pCpu->RunningIrq = true;
    pCpu->IPL = g_IrqTbl[Irq].IPL;
    g_IrqTbl[Irq].pHandler(pCtx);
    pCpu->RunningIrq = false;
    if (pCpu->QueuedIrqIdx > 0) {
        // Find next highest priority interrupt to run
        uint8_t IPL = 0;
        uint8_t HighestIrq = 0;
        IrqDescriptor *IrqDesc = 0;
        for (uint8_t i = 0; i < pCpu->QueuedIrqIdx; i++) {
            if (pCpu->QueuedIrqs[i] == 0xff) {
                if (i == pCpu->QueuedIrqIdx - 1) {
                    pCpu->QueuedIrqIdx--;
                    break;
                }
                continue; // IRQ 255 is not possible (only 224 entries on our array)
            }
            IrqDesc = &g_IrqTbl[pCpu->QueuedIrqs[i]];
            if (IrqDesc->IPL > IPL) {
                IPL = IrqDesc->IPL;
                HighestIrq = i;
            }
        }
        Irq = pCpu->QueuedIrqs[HighestIrq];
        pCpu->QueuedIrqs[HighestIrq] = 0xff;
        KxSendInt(pCpu->CpuNum, Irq);
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

void KxSendIntAll(uint8_t Irq) {
    #if defined (__x86_64__)
    KeLocalApicIpiAll(KeSmpGetCpu()->CpuNum, Irq + 32);
    #endif
}

void KxEndOfInt() {
    #if defined (__x86_64__)
    KeLocalApicEoi();
    #endif
}

void KxTimeInt(uint8_t Irq, uint64_t Ms) {
    #if defined (__x86_64__)
    KeLocalApicOneShot(Irq + 32, Ms);
    #endif
}
