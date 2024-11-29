#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <x86/madt.h>

void KeLocalApicInit();
void KeLocalApicWrite(uint32_t Register, uint64_t Data);
uint64_t KeLocalApicRead(uint32_t Register);
void KeLocalApicEoi();
void KeLocalApicIpi(uint32_t LocalApicId, uint32_t Data);
void KeLocalApicIpiAll(uint32_t LocalApicId, uint8_t Vector);
void KeLocalApicOneShot(uint32_t Vector, uint64_t Ms);
uint64_t KeLocalApicInitTimer();
uint32_t KeLocalApicGetID();

extern MadtIoApic *g_pMpIoApic;
void KeIoApicRemapIrq(MadtIoApic *pIoApic, uint8_t Irq, uint8_t Vec, bool Masked);
void KeIoApicInit();
