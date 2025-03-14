#include <bfs.h>
#include <alloc.h>
#include <string.h>
#include <printf.h>
#include <stdbool.h>

BfsDesc *FsCreateBfs(void(*DiskRead)(uint8_t*, uint32_t, uint32_t)) {
    BfsDesc *pBfs = (BfsDesc*)MmAlloc(sizeof(BfsDesc));

    BfsInfo *pInfo = (BfsInfo*)MmAlloc(sizeof(BfsInfo));
    DiskRead(pInfo, 0, 1);
    pBfs->pInfo = pInfo;
    pBfs->DiskRead = DiskRead;

    if (memcmp(pInfo->Magic, "BFS", 3)) {
        printf("FS: Invalid BFS Magic. got '%.*s'.\n", 3, pInfo->Magic);
        return NULL;
    }

    return pBfs;
}

BfsDir *FsBfsReadDir(BfsDesc *pBfs, uint32_t DirBlock, uint32_t Index) {
    uint32_t BlockSkips = Index / ((BLOCK_SIZE - 4) / sizeof(BfsDir));
    uint8_t *pBuffer = (uint8_t*)MmAlloc(BLOCK_SIZE);
    uint32_t CurrentBlock = DirBlock;
    pBfs->DiskRead(pBuffer, CurrentBlock, 1);
    uint32_t CurrentIndex = 0;
    for (int i = 0; i < BlockSkips; i++) {
        // Skip to the next block.
        uint32_t NextBlock = *((uint32_t*)(pBuffer + (BLOCK_SIZE - 4)));
        if (!NextBlock)
            return NULL;
        CurrentBlock = NextBlock;
        pBfs->DiskRead(pBuffer, CurrentBlock, 1);
        CurrentIndex += ((BLOCK_SIZE - 4) / sizeof(BfsDir));
    }
    // Find file
    int Offset = 0;
    BfsDir *pDir = (BfsDir*)pBuffer;
    while (CurrentIndex != Index) {
        Offset += sizeof(BfsDir);
        pDir = (BfsDir*)(pBuffer + Offset);
        CurrentIndex++;
    }
    if (pBuffer[Offset] == 0) {
        MmFree(pBuffer);
        return NULL;
    }
    BfsDir *pNewDir = (BfsDir*)MmAlloc(sizeof(BfsDir));
    memcpy(pNewDir, pDir, sizeof(BfsDir));
    MmFree(pBuffer);
    return pNewDir;
}

uint32_t FsBfsGetIndex(BfsDesc *pBfs, uint32_t DirBlock, char *pName) {
    uint8_t *pBuffer = (uint8_t*)MmAlloc(BLOCK_SIZE);
    BfsDir *pDir = (BfsDir*)pBuffer;
    uint32_t Offset = 0;
    uint32_t CurrentBlock = DirBlock;
    pBfs->DiskRead(pBuffer, DirBlock, 1);
    while (pBuffer[Offset] != 0) {
        if (Offset + sizeof(BfsDir) >= BLOCK_SIZE - 4) {
            Offset = 0;
            uint32_t NextBlock = *((uint32_t*)(pBuffer + (BLOCK_SIZE - 4)));
            if (!NextBlock)
                break;
            CurrentBlock = NextBlock;
            pBfs->DiskRead(pBuffer, CurrentBlock, 1);
            pDir = (BfsDir*)pBuffer;
        }
        if (!strcmp(pDir->Name, pName)) {
            uint32_t Index = pDir->Index;
            MmFree(pBuffer);
            return Index;
        }
        Offset += sizeof(BfsDir);
        pDir = (BfsDir*)(pBuffer + Offset);
    }
    MmFree(pBuffer);
    return 0;
}

BfsFile *FsBfsSearchFileTable(BfsDesc *pBfs, uint32_t Index) {
    uint32_t FileTableNum = Index / MAX_FT_ENTRY;
    uint8_t *pBuffer = (uint8_t*)MmAlloc(BLOCK_SIZE);
    BfsFileTable *pFileTable = (BfsFileTable*)pBuffer;
    pBfs->DiskRead(pBuffer, pBfs->pInfo->FirstFtBlock, 1);
    for (int i = 0; i < FileTableNum; i++) {
        uint32_t NextBlock = pFileTable->NextBlock;
        if (!NextBlock) {
            MmFree(pBuffer);
            return NULL;
        }
        pBfs->DiskRead(pBuffer, NextBlock, 1);
    }
    BfsFile *pFile = (BfsFile*)MmAlloc(sizeof(BfsFile));
    memcpy(pFile, &pFileTable->Entries[Index], sizeof(BfsFile));
    MmFree(pBuffer);
    return pFile;
}

BfsFile *FsBfsFindDir(BfsDesc *pBfs, uint32_t DirBlock, char *pName) {
    return FsBfsSearchFileTable(pBfs, FsBfsGetIndex(pBfs, DirBlock, pName));
}

void FsBfsReadBlockChain(BfsDesc *pBfs, uint8_t *pBuffer, uint32_t Block, uint32_t Size) {
    uint8_t Temp[BLOCK_SIZE];
    pBfs->DiskRead(Temp, Block, 1);
    uint32_t NextBlock = *((uint32_t*)(Temp + (BLOCK_SIZE - 4)));
    int i = 0;
    bool Reading = true;
    while (Reading) {
        uint32_t RemainingSize = (Size > (BLOCK_SIZE - 4) ? (BLOCK_SIZE - 4) : Size);
        memcpy(pBuffer + (i * (BLOCK_SIZE - 4)), Temp, RemainingSize);
        Size -= BLOCK_SIZE - 4;
        if (NextBlock) {
            pBfs->DiskRead(Temp, NextBlock, 1);
            NextBlock = *((uint32_t*)(Temp + (BLOCK_SIZE - 4)));
        } else {
            Reading = false;
        }
        i++;
    }
}
