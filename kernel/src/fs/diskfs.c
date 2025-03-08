#include <diskfs.h>
#include <string.h>
#include <fat32.h>
#include <ahci.h>
#include <pmm.h>
#include <vmm.h>

uint8_t *pDiskBuffer = NULL;
Fat32FS *pDiskFS = NULL;

void DiskFat32ReadSector(uint8_t *pBuffer, uint64_t Sector, uint64_t Size) {
    BlkAhciRead(g_pAhciPorts[0], Sector, DIV_ROUND_UP(Size, 512), pDiskBuffer);
    memcpy(pBuffer, pDiskBuffer, Size);
}

int DiskFat32Mount() {
    pDiskBuffer = MmVirtAllocatePages(g_pKernelPageMap, 10, MM_READ | MM_WRITE);
    pDiskFS = FsFatInit(DiskFat32ReadSector);
    return 0;
}