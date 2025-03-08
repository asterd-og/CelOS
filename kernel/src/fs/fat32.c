#include <fat32.h>
#include <printf.h>
#include <alloc.h>

uint64_t FsFatClusterToSector(Fat32FS *pFat, uint64_t Cluster) {
    return ((Cluster - 2) * pFat->pBPB->SectorsPerCluster) + (pFat->pBPB->ReservedSectors + (pFat->pBPB->FatCount * pFat->pBPB->SectorsPerFat));
}

Fat32FS *FsFatInit(void(*DiskReadSector)(uint8_t *pBuffer, uint64_t Sector, uint64_t Size)) {
    Fat32FS *pFat = (Fat32FS*)MmAlloc(sizeof(Fat32FS));
    pFat->pBPB = (Fat32BPB*)MmAlloc(sizeof(Fat32BPB));
    DiskReadSector(pFat->pBPB, 0, 512);
    pFat->pFsInfo = (Fat32FsInfo*)MmAlloc(sizeof(Fat32FS));
    DiskReadSector(pFat->pFsInfo, pFat->pBPB->FsInfoSector, sizeof(Fat32FS));

    uint64_t RootSector = FsFatClusterToSector(pFat, pFat->pBPB->RootDirCluster);

    uint8_t *pBuffer = (uint8_t*)MmAlloc(512);
    DiskReadSector(pBuffer, RootSector, 512);

    printf("Listing root files:\n");

    for (int i = 0; i < 16; i++, pBuffer += sizeof(Fat32Dir)) {
        if (i == 5) break;
        if (pBuffer[0] == 0) break;
        if (pBuffer[11] == 0x0f) continue;
        Fat32Dir *pRootDir = (Fat32Dir*)pBuffer;
        if (pRootDir->Attributes & 0x02) continue;
        if (pRootDir->Attributes & 0x20) {
            printf("%d %.*s.%.*s\n", i, 8, pRootDir->FileName, 3, pRootDir->FileName + 8);
        } else {
            printf("%.*s\n", 11, pRootDir->FileName);
        }
    }
    Fat32Dir *pFile = (Fat32Dir*)pBuffer;

    uint32_t Cluster = ((uint32_t)pFile->HiFirstCluster << 16)
                   | ((uint32_t)pFile->LoFirstCluster);

    char *pFileBuffer = MmAlloc(pFile->FileSize);
    DiskReadSector(pFileBuffer, FsFatClusterToSector(pFat, Cluster), pFile->FileSize);
    printf("%s\n", pFileBuffer);
}
