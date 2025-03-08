#pragma once

#include <stdint.h>
#include <stddef.h>

#define FS_FILE 1
#define FS_DIR 2

typedef struct {
    uint32_t Inode;
    char Name[256];
} DirEnt;

typedef struct Vnode {
    uint16_t Type;
    char Name[256];
    size_t Size; // In bytes
    uint32_t Inode;
    size_t (*Read)(struct Vnode *pNode, uint8_t *pBuffer, size_t Length);
    size_t (*Write)(struct Vnode *pNode, uint8_t *pBuffer, size_t Length);
    DirEnt *(*ReadDir)(struct Vnode *pNode, uint32_t Index);
    struct Vnode *(*FindDir)(struct Vnode *pNode, char *pName);
} Vnode;

typedef struct {
    Vnode *pNode;
    int Offset;
} FileDescriptor;

extern Vnode *pFsRoot;

Vnode *FsMount(
    size_t (*Read)(struct Vnode *pNode, uint8_t *pBuffer, size_t Length),
    size_t (*Write)(Vnode *pNode, uint8_t *pBuffer, size_t Length),
    DirEnt *(*ReadDir)(Vnode *pNode, uint32_t Index),
    Vnode *(*FindDir)(Vnode *pNode, char *pName),
    int Inode
);

Vnode *FsFindMountPoint(char Letter);
Vnode *FsFind(char *pPath);

size_t FsRead(Vnode *pNode, uint8_t *pBuffer, size_t Length);
size_t FsWrite(Vnode *pNode, uint8_t *pBuffer, size_t Length);
DirEnt *FsReadDir(Vnode *pNode, uint32_t Index);
Vnode *FsFindDir(Vnode *pNode, char *pName);
