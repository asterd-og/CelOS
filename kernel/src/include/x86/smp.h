#pragma once

#include <stdint.h>
#include <stddef.h>
#include <vmm.h>
#include <limine.h>

typedef struct {
    uint64_t LocalApicTicks;
    uint32_t LocalApicID;
} ArchCpuInfo;

void KeArchSmpCpuInit(struct limine_smp_info *pSmpInfo, ArchCpuInfo *pCpuInfo);
void KeArchSmpInit(ArchCpuInfo *pCpuInfo);
uint64_t KeArchSmpGetCpuNum();
