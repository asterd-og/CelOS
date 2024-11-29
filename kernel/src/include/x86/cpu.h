#pragma once

#include <stdint.h>

uint64_t KeRdmsr(uint32_t Msr);
void KeWrmsr(uint32_t Msr, uint64_t Val);