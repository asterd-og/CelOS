/*
Ext2 driver written by Astrido
*/

#pragma once

#include <stdint.h>
#include <stddef.h>

#define AE2_OK 0
#define AE2_WRONG_MAGIC 1
#define AE2_NOT_IMPL 2
#define AE2_FAIL 3

#define EXT2_ROOT_INODE 2

#define EXT2_FT_UNKNOWN 0
#define EXT2_FT_FILE 1
#define EXT2_FT_DIR 2
#define EXT2_FT_CHRDEV 3
#define EXT2_FT_BLKDEV 4
#define EXT2_FT_FIFO 5
#define EXT2_FT_SOCK 6
#define EXT2_FT_SYMLIN 7

#define DIV_ROUND_UP(x, y) (x + (y - 1)) / y
#define ALIGN_UP(x, y) (DIV_ROUND_UP(x, y) * y)

#define MAX(x, y) (x > y ? y : x)

typedef struct Ext2Disk {
    uint32_t BlockSize;
    void(*ReadBlock)(struct Ext2Disk*, uint8_t*, size_t, size_t);
    void(*WriteBlock)(struct Ext2Disk*, uint8_t*, size_t, size_t);
} Ext2Disk;

typedef struct {
    uint32_t InodeCount;
    uint32_t BlockCount;
    uint32_t ResvBlockCount;
    uint32_t FreeBlockCount;
    uint32_t FreeInodeCount;
    uint32_t FirstDataBlock;
    uint32_t LogBlockSize;
    uint32_t LogFragSize;
    uint32_t BlocksPerGroup;
    uint32_t FragsPerGroup;
    uint32_t InodesPerGroup;
    uint32_t LastMountTime;
    uint32_t LastWriteTime;
    uint16_t MountCount;
    uint16_t MaxMountCount;
    uint16_t Magic;
    uint16_t State;
    uint16_t Errors;
    uint16_t MinorRevLevel;
    uint32_t LastCheck;
    uint32_t CheckInterval;
    uint32_t CreatorOS;
    uint32_t MajorVer;
    uint16_t DefResUID;
    uint16_t DefResGID;
    // EXT2_DYNAMIC_REV
    uint32_t FirstInode;
    uint16_t InodeSize;
    uint16_t BlockGroupNum;
    uint32_t OptionalFeatures;
    uint32_t RequiredFeatures;
    uint32_t MountFeatures;
    uint64_t UUID[2];
    char VolumeName[16];
    char LastMounted[64];
    uint32_t AlgoBitmap;
    uint8_t PreallocBlocks;
    uint8_t PreallocDirBlocks;
    uint16_t Unused;
    uint64_t JournalUUID[2];
    uint32_t JournalINum;
    uint32_t JournalDev;
    uint32_t LastOrphan;
    uint32_t HashSeed[4];
    uint8_t DefHashVersion;
    uint8_t Padding[3];
    uint32_t DefaultMountOptions;
    uint32_t FirstMetaBg;
    uint8_t UnusedExt[760];
} Ext2SuperBlock;

typedef struct {
    uint16_t Mode;
    uint16_t UID;
    uint32_t Size;
    uint32_t AccessTime;
    uint32_t CreateTime;
    uint32_t ModifyTime;
    uint32_t DeleteTime;
    uint16_t GID;
    uint16_t LinksCount;
    uint32_t Blocks;
    uint32_t Flags;
    uint32_t OSD1;
    uint32_t DirectBlocks[12];
    uint32_t SinglyIndirectBlockPtr;
    uint32_t DoublyIndirectBlockPtr;
    uint32_t TriplyIndirectBlockPtr;
    uint32_t GenerationNum;
    uint32_t FileAcl;
    uint32_t DirAcl;
    uint32_t FragBlockAddr;
    uint32_t OSD2[3];
} Ext2Inode;

typedef struct {
    uint32_t BlockBitmap;
    uint32_t InodeBitmap;
    uint32_t InodeTable;
    uint16_t FreeBlockCount;
    uint16_t FreeInodeCount;
    uint16_t UsedDirCount;
    uint16_t Padding;
    uint32_t Reserved[3];
} Ext2BlockGroupDesc;

typedef struct {
    uint32_t Inode;
    uint16_t EntryLen;
    uint8_t NameLen;
    uint8_t FileType;
    char Name[256];
} Ext2Directory;

typedef struct {
    uint32_t InodeSize;
    uint32_t BlockSize;
    Ext2SuperBlock *pSuperBlock;
    Ext2Disk *pDisk;

    uint32_t BlockGroupCount;
    Ext2BlockGroupDesc *pBlockGroups;
} Ext2FS;

Ext2FS *Ext2NewFS(Ext2Disk *pDisk);
int Ext2Init(Ext2FS *pFS);
int Ext2DestroyFS(Ext2FS *pFS);

int Ext2ReadInode(Ext2FS *pFS, uint32_t InodeNum, Ext2Inode *pInode);
int Ext2ReadInodeBlocks(Ext2FS *pFS, Ext2Inode *pInode, uint8_t *pBuffer, uint32_t BlockCount);
int Ext2ReadInodeBlocksOffset(Ext2FS *pFS, Ext2Inode *pInode, uint8_t *pBuffer, uint32_t BlockStart, uint32_t BlockCount);

uint32_t Ext2FindInInode(Ext2FS *pFS, Ext2Inode *pInode, char *pszName);

