/*
Ext2 driver written by Astrido
*/

#include "ext2.h"
#include <string.h>
#include <alloc.h>
#include <stdbool.h>

Ext2FS *Ext2NewFS(Ext2Disk *pDisk) {
    Ext2FS *pFS = (Ext2FS*)MmAlloc(sizeof(Ext2FS));
    pFS->pDisk = pDisk;
    pDisk->BlockSize = 1024;
    pFS->pSuperBlock = (Ext2SuperBlock*)MmAlloc(sizeof(Ext2SuperBlock));
    return pFS;
}

int Ext2Init(Ext2FS *pFS) {
    pFS->pDisk->ReadBlock(pFS->pDisk, (uint8_t*)pFS->pSuperBlock, 1, 1);
    Ext2SuperBlock *pSuperBlock = pFS->pSuperBlock;
    if (pSuperBlock->Magic != 0xef53)
        return AE2_WRONG_MAGIC;
    uint32_t BlockSize = 1024 << pSuperBlock->LogBlockSize;
    pFS->pDisk->BlockSize = BlockSize;
    pFS->BlockSize = BlockSize;
    
    // For each block group in the file system, such a group_desc is created.
    pFS->BlockGroupCount = pSuperBlock->BlockCount / pSuperBlock->BlocksPerGroup;
    uint32_t BlockGroupsSize = sizeof(Ext2BlockGroupDesc) * pFS->BlockGroupCount;
    uint32_t BlockGroupBlocks = DIV_ROUND_UP(BlockGroupsSize, pFS->BlockSize);
    pFS->pBlockGroups = (Ext2BlockGroupDesc*)MmAlloc(BlockGroupBlocks * pFS->BlockSize);
    memset(pFS->pBlockGroups, 0, BlockGroupsSize);

    pFS->pDisk->ReadBlock(pFS->pDisk, (uint8_t*)pFS->pBlockGroups, (pFS->BlockSize > 1024 ? 1 : 2), BlockGroupBlocks);
    pFS->InodeSize = (pSuperBlock->MajorVer == 1 ? pSuperBlock->InodeSize : 256);

    return AE2_OK;
}

int Ext2DestroyFS(Ext2FS *pFS) {
    MmFree(pFS->pSuperBlock);
    MmFree(pFS->pBlockGroups);
    MmFree(pFS);
    return AE2_OK;
}

int Ext2ReadInode(Ext2FS *pFS, uint32_t InodeNum, Ext2Inode *pInode) {
    uint32_t BlockGroup = (InodeNum - 1) / pFS->pSuperBlock->InodesPerGroup;
    uint32_t InodeIndex = (InodeNum - 1) % pFS->pSuperBlock->InodesPerGroup;
    uint32_t BlockGroupIndex = (InodeIndex  * pFS->InodeSize) / pFS->BlockSize;
    uint32_t InodeTable = pFS->pBlockGroups[BlockGroup].InodeTable;
    uint8_t *pBuffer = (uint8_t*)MmAlloc(pFS->BlockSize);
    pFS->pDisk->ReadBlock(pFS->pDisk, pBuffer, InodeTable + BlockGroupIndex, 1);
    memcpy(pInode, pBuffer + ((InodeIndex % (pFS->BlockSize / pFS->InodeSize)) * pFS->InodeSize), pFS->InodeSize);
    MmFree(pBuffer);
    return AE2_OK;
}

// Returns amount of read blocks
int Ext2ReadSinglyIndirectBlock(Ext2FS *pFS, uint8_t *pBuffer, uint32_t IndirectBlock, uint32_t MaxBlocks) {
    size_t BlockCount = pFS->BlockSize / sizeof(uint32_t);
    size_t IndirectBlockSize = BlockCount * pFS->BlockSize;
    uint32_t *pBlockArr = (uint32_t*)MmAlloc(pFS->BlockSize);
    memset(pBlockArr, 0, pFS->BlockSize);
    pFS->pDisk->ReadBlock(pFS->pDisk, (uint8_t*)pBlockArr, IndirectBlock, 1);

    uint32_t ReadBlocks = 0;
    for (int i = 0; i < MaxBlocks; i++) {
        pFS->pDisk->ReadBlock(pFS->pDisk, pBuffer + (i * pFS->BlockSize), pBlockArr[i], 1);
        ReadBlocks++;
    }
    MmFree(pBlockArr);
    return ReadBlocks;
}

int Ext2ReadDoublyIndirectBlock(Ext2FS *pFS, uint8_t *pBuffer, uint32_t IndirectBlock, uint32_t MaxBlocks) {
    size_t BlockCount = pFS->BlockSize / sizeof(uint32_t);
    size_t IndirectBlockSize = BlockCount * pFS->BlockSize;
    uint32_t *pBlockArr = (uint32_t*)MmAlloc(pFS->BlockSize);
    memset(pBlockArr, 0, pFS->BlockSize);
    pFS->pDisk->ReadBlock(pFS->pDisk, (uint8_t*)pBlockArr, IndirectBlock, 1);

    uint32_t ReadBlocks = 0;
    uint32_t Remaining = MaxBlocks;
    for (int i = 0; i < BlockCount; i++) {
        uint32_t IndirectReadCount = Ext2ReadSinglyIndirectBlock(pFS,
            pBuffer + (i * IndirectBlockSize), pBlockArr[i], MAX(Remaining, BlockCount));
        ReadBlocks += IndirectReadCount;
        Remaining -= IndirectReadCount;
        if (Remaining == 0)
            break;
    }
    MmFree(pBlockArr);
    return ReadBlocks;
}

int Ext2ReadTriplyIndirectBlock(Ext2FS *pFS, uint8_t *pBuffer, uint32_t IndirectBlock, uint32_t MaxBlocks) {
    uint32_t BlockCount = pFS->BlockSize / sizeof(uint32_t);
    uint32_t DoublyIndirectBlockCount = BlockCount * BlockCount;
    uint32_t DoublyIndirectSize = DoublyIndirectBlockCount * pFS->BlockSize;
    uint32_t *pBlockArr = (uint32_t*)MmAlloc(pFS->BlockSize);
    memset(pBlockArr, 0, pFS->BlockSize);
    pFS->pDisk->ReadBlock(pFS->pDisk, (uint8_t*)pBlockArr, IndirectBlock, 1);

    uint32_t ReadBlocks = 0;
    uint32_t Remaining = MaxBlocks;
    for (int i = 0; i < BlockCount; i++) {
        uint32_t IndirectReadCount = Ext2ReadDoublyIndirectBlock(pFS,
            pBuffer + (i * DoublyIndirectSize), pBlockArr[i], Remaining);
        ReadBlocks += IndirectReadCount;
        Remaining -= IndirectReadCount;
        if (Remaining == 0)
            break;
    }
    MmFree(pBlockArr);
    return ReadBlocks;
}

int Ext2ReadInodeBlocks(Ext2FS *pFS, Ext2Inode *pInode, uint8_t *pBuffer, uint32_t BlockCount) {
    uint32_t CurrentBlockCount = BlockCount;
    for (uint32_t i = 0; i < 12 && i < BlockCount; i++) {
        pFS->pDisk->ReadBlock(pFS->pDisk, pBuffer + (i * pFS->BlockSize), pInode->DirectBlocks[i], 1);
        CurrentBlockCount--;
    }
    uint32_t BlockCountInU32 = pFS->BlockSize / sizeof(uint32_t);
    if (CurrentBlockCount > 0) {
        uint32_t IndirectBlockCount = BlockCountInU32;
        uint32_t ReadBlocks = Ext2ReadSinglyIndirectBlock(pFS,
            pBuffer + (12 * pFS->BlockSize),
            pInode->SinglyIndirectBlockPtr, MAX(IndirectBlockCount, CurrentBlockCount));
        CurrentBlockCount -= ReadBlocks;
    }
    if (CurrentBlockCount > 0) {
        uint32_t SinglyIndirectBlockSize = BlockCountInU32 * pFS->BlockSize;
        uint32_t DoublyIndirectBlockCount = BlockCountInU32 * BlockCountInU32;
        uint32_t ReadBlocks = Ext2ReadDoublyIndirectBlock(pFS,
            pBuffer + ((12 * pFS->BlockSize) + SinglyIndirectBlockSize),
            pInode->DoublyIndirectBlockPtr, MAX(DoublyIndirectBlockCount, CurrentBlockCount));
        CurrentBlockCount -= ReadBlocks;
    }
    if (CurrentBlockCount > 0) {
        uint32_t SinglyIndirectBlockSize = BlockCountInU32 * pFS->BlockSize;
        uint32_t DoublyIndirectBlockSize = (BlockCountInU32 * BlockCountInU32) * pFS->BlockSize;
        uint32_t ReadBlocks = Ext2ReadTriplyIndirectBlock(pFS,
            pBuffer + ((12 * pFS->BlockSize) + SinglyIndirectBlockSize + DoublyIndirectBlockSize),
            pInode->TriplyIndirectBlockPtr, CurrentBlockCount);
        CurrentBlockCount -= ReadBlocks;
    }
    return AE2_OK;
}

uint32_t Ext2FindInInode(Ext2FS *pFS, Ext2Inode *pInode, char *pszName) {
    size_t NameLen = 0;
    for (int i = 0; pszName[i] != 0; i++) NameLen++;
    if (NameLen > 256) return 0;
    uint32_t InodeBlockCount = DIV_ROUND_UP(pInode->Size, pFS->BlockSize);
    uint8_t *pBuffer = (uint8_t*)MmAlloc(InodeBlockCount * pFS->BlockSize);
    Ext2ReadInodeBlocks(pFS, pInode, pBuffer, InodeBlockCount);
    Ext2Directory *pDir = (Ext2Directory*)pBuffer;

    uint32_t Offset = 0;
    while (pDir->Inode != 0) {
        if (!memcmp(pDir->Name, pszName, NameLen)) {
            uint32_t Inode = pDir->Inode;
            MmFree(pBuffer);
            return Inode;
        }
        Offset += pDir->EntryLen;
        pDir = (Ext2Directory*)(pBuffer + Offset);
    }

    MmFree(pBuffer);
    return 0;
}

