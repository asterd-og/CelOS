#include <x86/pit.h>
#include <x86/idt.h>
#include <x86/apic.h>
#include <x86/ports.h>
#include <printf.h>

uint64_t g_PitCounter = 0;

void KePitHandler(void) {
    g_PitCounter++;
    KeLocalApicEoi();
}

void KePitInit() {
    uint16_t Div = 1193180 / 1000; // 1000hz = 1 ms/tick
    KeWritePort8(0x43, 0b110110);
    KeWritePort8(0x40, Div);
    KeWritePort8(0x40, Div >> 8);
    KeInstallIrq(0, KePitHandler, true);
}

void KePitSleep(uint64_t Ms) {
    uint64_t SleepUntil = g_PitCounter + Ms;
    while (g_PitCounter <= SleepUntil)
        __asm__ volatile ("pause");
}
