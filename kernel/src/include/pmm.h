#pragma once

#include <stdint.h>
#include <stddef.h>
#include <kernel.h>
#include <limine.h>

#define PAGE_SIZE 4096

#define HIGHER_HALF(x) ((void*)((uint64_t)x) + HhdmOffset)
#define PHYSICAL(x) ((void*)((uint64_t)x) - HhdmOffset)

#define DIV_ROUND_UP(x, y) (x + (y - 1)) / y
#define ALIGN_UP(x, y) DIV_ROUND_UP(x, y) * y
#define ALIGN_DOWN(x, y) (x / y) * y

extern struct limine_memmap_response *g_pMmapResponse;

void MmPhysInit();
void *MmPhysAllocatePage();
void MmPhysFreePage(void *pAddress);
