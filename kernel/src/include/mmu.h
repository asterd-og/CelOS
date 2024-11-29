#pragma once

#include <stdint.h>
#include <stddef.h>

void MmArchVirtInit();
void MmArchSwitchPageMap(uint64_t *pPageMap);
void MmArchVirtMap(uint64_t *pPageMap, uint64_t VirtualAddress, uint64_t PhysicalAddress, uint64_t Flags);
void MmArchVirtUnmap(uint64_t *pPageMap, uint64_t VirtualAddress);
uint64_t MmArchGetPagePhysicalAddress(uint64_t *pPageMap, uint64_t VirtualAddress);
