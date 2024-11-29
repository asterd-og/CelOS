/*

Memory allocator written by: Astrido

*/

#pragma once

#include <stdint.h>
#include <stddef.h>

void MmAllocInit();
void *MmAlloc(size_t size);
void MmFree(void *ptr);
void MmAllocPrint();
void MmAllocDestroy();