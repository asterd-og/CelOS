#include <x86/apic.h>
#include <x86/cpu.h>
#include <x86/idt.h>
#include <x86/pit.h>
#include <smp.h>
#include <pmm.h>
#include <vmm.h>
#include <assert.h>
#include <printf.h>

uint32_t *g_pLocalApicAddr = NULL;
bool g_X2Apic = false;

// TODO: Suppor xAPIC

void KeLocalApicInit() {
    ASSERT(g_pLocalApicPhysAddr);
    g_pLocalApicAddr = HIGHER_HALF(g_pLocalApicPhysAddr); // Mapped (virtual) LAPIC address
    uint64_t Flags = KeRdmsr(0x1b);
    Flags |= 0x800; // Enable LAPIC
    uint32_t a = 1, b = 0, c = 0, d = 0;
    __asm__ __volatile__ ("cpuid"
                         : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                         : "a"(a));
    if (c & (1 << 21)) {
        g_X2Apic = true;
        Flags |= 0x400; // Enable x2apic
    }
    KeWrmsr(0x1b, Flags);
    KeLocalApicWrite(0xf0, KeLocalApicRead(0xf0) | 0x100);
}

void KeLocalApicWrite(uint32_t Register, uint64_t Data) {
    if (g_X2Apic) {
        Register = (Register >> 4) + 0x800;
        KeWrmsr(Register, Data);
        return;
    }
    uint64_t Addr = (uint64_t)g_pLocalApicAddr + Register;
    *((volatile uint32_t*)Addr) = Data;
}

uint64_t KeLocalApicRead(uint32_t Register) {
    if (g_X2Apic) {
        Register = (Register >> 4) + 0x800;
        return KeRdmsr(Register);
    }
    uint64_t Addr = (uint64_t)g_pLocalApicAddr + Register;
    return *((volatile uint32_t*)Addr);
}

void KeLocalApicEoi() {
    KeLocalApicWrite(0x00b0, 0);
}

void KeLocalApicIpi(uint32_t LocalApicId, uint32_t Data) {
    if (g_X2Apic)
        KeLocalApicWrite(0x0300, ((uint64_t)LocalApicId << 32) | Data);
    else {
        KeLocalApicWrite(0x0310, LocalApicId << 24);
        KeLocalApicWrite(0x0300, Data);
    }
}

void KeLocalApicIpiAll(uint32_t LocalApicId, uint8_t Vector) {
    KeLocalApicIpi(LocalApicId, Vector | 0x00080000);
}

void KeLocalApicOneShot(uint32_t Vector, uint64_t Ms) {
    KeLocalApicWrite(0x320, 0x00010000);
    KeLocalApicWrite(0x380, 0);
    KeLocalApicWrite(0x3e0, 0x3);
    KeLocalApicWrite(0x380, Ms * KeSmpGetCpu()->ArchInfo.LocalApicTicks);
    KeLocalApicWrite(0x320, Vector);
}

void KeLocalApicStopTimer() {
    // Mask timer interrupt
    KeLocalApicWrite(0x380, 0);
    KeLocalApicWrite(0x320, 0x00010000);
}

uint64_t KeLocalApicInitTimer() {
    KeLocalApicWrite(0x3e0, 0x3);        // Divisor = 16
    KeLocalApicWrite(0x380, 0xffffffff); // Init counter = -1
    KePitSleep(1);
    KeLocalApicWrite(0x320, 0x00010000); // Mask LVT timer
    uint32_t InitCnt = 0xffffffff - KeLocalApicRead(0x390);
    return InitCnt;
}

uint32_t KeLocalApicGetID() {
    uint32_t ID = KeLocalApicRead(0x0020);
    if (!g_X2Apic) ID >>= 24;
    return ID;
}

// IoApic

MadtIoApic *g_pMpIoApic = NULL;
#define REDTBL(n) (0x10 + 2 * n)

void KeIoApicWrite(uint64_t pIoApicAddr, uint8_t Register, uint32_t Data) {
    *(uint32_t volatile*)pIoApicAddr = Register;
    *(uint32_t volatile*)((uint8_t*)pIoApicAddr + 0x10) = Data;
}

uint32_t KeIoApicRead(uint64_t pIoApicAddr, uint8_t Register) {
    *(uint32_t volatile*)pIoApicAddr = Register;
    return *(uint32_t volatile*)((uint8_t*)pIoApicAddr + 0x10);
}

void KeIoApicRemapGsi(MadtIoApic *pIoApic, uint32_t Lapic, uint32_t Gsi, uint8_t Vec, bool PinPolarity, bool TriggerMode, bool Masked) {
    uint64_t Value = Vec;
    if (PinPolarity) Value |= (1 << 13);
    if (TriggerMode) Value |= (1 << 15);
    if (Masked) Value |= (1 << 16);
    Value |= (uint64_t)Lapic << 56;
    KeIoApicWrite((uint64_t)HIGHER_HALF(pIoApic->IoApicPhysAddress), REDTBL(Gsi), (uint32_t)Value);
    KeIoApicWrite((uint64_t)HIGHER_HALF(pIoApic->IoApicPhysAddress), REDTBL(Gsi)+1, (uint32_t)(Value >> 32));
}

void KeIoApicRemapIrq(MadtIoApic *pIoApic, uint8_t Irq, uint8_t Vec, bool Masked) {
    MadtIoApicIntSrcOvr *pSrcOvr;
    for (uint64_t i = 0; i < g_IoApicIntSrcOvrSize; i++) {
        pSrcOvr = g_pIoApicIntSrcOvrList[i];
        if (pSrcOvr->IrqSource == Irq) {
            bool Trigger = (Irq == 11 ? 1 : pSrcOvr->Flags & (1 << 3));
            uint32_t Gsi = pSrcOvr->Gsi;
            if (pIoApic->GsiBase > 0) Gsi -= pIoApic->GsiBase; // In case of multiple IOAPICs in a system
            KeIoApicRemapGsi(pIoApic, 0, Gsi, Vec, pSrcOvr->Flags & (1 << 1),
                Trigger, Masked);
            break;
        }
        if (i + 1 == g_IoApicIntSrcOvrSize) {
            KeIoApicRemapGsi(pIoApic, 0, Irq, Vec, false, false, Masked);
            break;
        }
    }
}

void KeIoApicInit() {
    g_pMpIoApic = g_pIoApicList[0];
    ASSERT(g_pMpIoApic);
    uint32_t Value = KeIoApicRead((uint64_t)HIGHER_HALF(g_pMpIoApic->IoApicPhysAddress), 0x01);
    uint32_t Count = ((Value >> 16) & 0xFF) + 1;
    for (uint8_t i = 0; i < Count; i++) {
        uint32_t Low = KeIoApicRead((uint64_t)HIGHER_HALF(g_pMpIoApic->IoApicPhysAddress), REDTBL(i));
        // Set masked bit on REDTBL entry.
        KeIoApicWrite((uint64_t)HIGHER_HALF(g_pMpIoApic->IoApicPhysAddress), REDTBL(i), Low | (1 << 16));
    }
}
