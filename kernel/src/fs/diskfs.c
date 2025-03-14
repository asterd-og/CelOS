#include <diskfs.h>
#include <assert.h>
#include <vfs.h>
#include <string.h>
#include <printf.h>
#include <bfs.h>
#include <ahci.h>
#include <pmm.h>
#include <vmm.h>
#include <alloc.h>

uint8_t *pDiskBuffer = NULL;
BfsDesc *pDiskBfs = NULL;

void DiskBfsReadBlock(uint8_t *pBuffer, uint32_t Block, uint32_t Count) {
    BlkAhciRead(g_pAhciPorts[0], Block * (BLOCK_SIZE / 512), Count * (BLOCK_SIZE / 512), pDiskBuffer);
    memcpy(pBuffer, pDiskBuffer, Count * BLOCK_SIZE);
}

size_t DiskBfsRead(Vnode *pNode, uint8_t *pBuffer, size_t Length) {
    ASSERT(pNode->Type == FS_FILE);
    uint32_t Block = pNode->Inode;
    FsBfsReadBlockChain(pDiskBfs, pBuffer, Block, Length);
    return Length;
}

size_t DiskBfsWrite(Vnode *pNode, uint8_t *pBuffer, size_t Length) {
    return 0;
}

DirEnt *DiskBfsReadDir(Vnode *pNode, uint32_t Index) {
    ASSERT(pNode->Type == FS_DIR);
    uint32_t Block = pNode->Inode;
    BfsDir *pBfsDir = FsBfsReadDir(pDiskBfs, Block, Index);
    if (!pBfsDir)
        return NULL;
    DirEnt *pDir = (DirEnt*)MmAlloc(sizeof(DirEnt));
    pDir->Inode = pBfsDir->Index;
    memcpy(pDir->Name, pBfsDir->Name, 81);
    MmFree(pBfsDir);
    return pDir;
}

Vnode *DiskBfsFindDir(Vnode *pNode, char *pName) {
    BfsFile *pBfsFile = FsBfsFindDir(pDiskBfs, pNode->Inode, pName);
    if (!pBfsFile)
        return NULL;
    Vnode *pNewNode = (Vnode*)MmAlloc(sizeof(Vnode));
    if (pBfsFile->Attributes & BFS_FILE) pNewNode->Type = FS_FILE;
    else if (pBfsFile->Attributes & BFS_DIR) pNewNode->Type = FS_DIR;
    memcpy(pNewNode->Name, pName, strlen(pName) + 1);
    pNewNode->Size = pBfsFile->Size;
    pNewNode->Inode = pBfsFile->FirstBlock;
    pNewNode->Read = DiskBfsRead;
    pNewNode->Write = DiskBfsWrite;
    pNewNode->ReadDir = DiskBfsReadDir;
    pNewNode->FindDir = DiskBfsFindDir;
    MmFree(pBfsFile);
    return pNewNode;
}

int DiskBfsMount() {
    pDiskBuffer = MmVirtAllocatePages(g_pKernelPageMap, 10, MM_READ | MM_WRITE);
    pDiskBfs = FsCreateBfs(DiskBfsReadBlock);
    FsMount(DiskBfsRead, DiskBfsWrite,
            DiskBfsReadDir, DiskBfsFindDir, pDiskBfs->pInfo->RootDirBlock);
    return (pDiskBfs ? 0 : 1);
}