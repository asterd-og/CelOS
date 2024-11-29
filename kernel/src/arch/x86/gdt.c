#include <x86/gdt.h>

GdtTable g_GdtTable = {
    {
        0x0000000000000000,
        0x00af9b000000ffff, // 0x08 64 bit cs (code)
        0x00af93000000ffff, // 0x10 64 bit ss (data)
        0x00affb000000ffff, // 0x20 user mode cs (code)
        0x00aff3000000ffff, // 0x18 user mode ss (data)
    }
};

GdtDescriptor g_GdtDesc;

void KeGdtReloadSeg();

void KeGdtInit() {
    g_GdtDesc.Size = (sizeof(g_GdtTable)) - 1;
    g_GdtDesc.Address = (uint64_t)&g_GdtTable;
    __asm__ volatile ("lgdt %0" : : "m"(g_GdtDesc) : "memory");
    KeGdtReloadSeg();
}
