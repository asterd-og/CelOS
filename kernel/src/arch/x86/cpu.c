#include <x86/cpu.h>

uint64_t KeRdmsr(uint32_t Msr) {
    uint32_t Low;
    uint32_t High;
    __asm__ volatile ("rdmsr" : "=a"(Low), "=d"(High) : "c"(Msr));
    return ((uint64_t)High << 32) | Low;
}

void KeWrmsr(uint32_t Msr, uint64_t Val) {
    __asm__ volatile ("wrmsr" : : "a"((uint64_t)Val), "d"((uint32_t)(Val >> 32)), "c"(Msr));
}