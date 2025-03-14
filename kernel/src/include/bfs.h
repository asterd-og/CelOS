#pragma once

#include <stdint.h>
#include <stddef.h>

#define BLOCK_SIZE 1024
#define MAX_FT_ENTRY 1020 / sizeof(BfsFile)

#define BFS_FILE 0x1
#define BFS_DIR 0x2
#define BFS_HIDDEN 0x4

typedef struct {
    char Magic[3];
    uint32_t AvailableBlocks;
    uint32_t LastFreeBlock;
    uint32_t RootDirBlock;
    uint32_t FirstFtBlock;
    char Reserved[1011];
} __attribute__((packed)) BfsInfo;

typedef struct {
    char Name[81];
    uint32_t Index; // Index within the file table.
} __attribute__((packed)) BfsDir;

typedef struct {
    uint8_t Attributes;
    uint8_t Reserved;
    uint32_t FirstBlock;
    uint32_t Size;
} __attribute__((packed)) BfsFile;

typedef struct {
    BfsFile Entries[MAX_FT_ENTRY];
    uint32_t NextBlock;
} __attribute__((packed)) BfsFileTable;

typedef struct {
    BfsInfo *pInfo;
    void(*DiskRead)(uint8_t *pBuffer, uint32_t Block, uint32_t Count);
} BfsDesc;

BfsDesc *FsCreateBfs(void(*DiskRead)(uint8_t*, uint32_t, uint32_t));
BfsDir *FsBfsReadDir(BfsDesc *pBfs, uint32_t DirBlock, uint32_t Index);
BfsFile *FsBfsFindDir(BfsDesc *pBfs, uint32_t DirBlock, char *pName);
void FsBfsReadBlockChain(BfsDesc *pBfs, uint8_t *pBuffer, uint32_t Block, uint32_t Size);