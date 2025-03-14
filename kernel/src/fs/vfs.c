#include <vfs.h>
#include <alloc.h>
#include <printf.h>
#include <string.h>

Vnode *pMountPoints[26] = {0};
int MountedCount = 0;

Vnode *pFsRoot = NULL;

Vnode *FsMount(
    size_t (*Read)(struct Vnode *pNode, uint8_t *pBuffer, size_t Length),
    size_t (*Write)(Vnode *pNode, uint8_t *pBuffer, size_t Length),
    DirEnt *(*ReadDir)(Vnode *pNode, uint32_t Index),
    Vnode *(*FindDir)(Vnode *pNode, char *pName),
    int Inode
) {
    if (MountedCount == 25)
        return NULL;
    Vnode *pNode = (Vnode*)MmAlloc(sizeof(Vnode));
    pNode->Type = FS_DIR;
    pNode->Inode = Inode;
    pNode->Name[0] = 'A' + MountedCount;
    pNode->Name[1] = 0;
    pNode->Read = Read;
    pNode->Write = Write;
    pNode->ReadDir = ReadDir;
    pNode->FindDir = FindDir;
    pMountPoints[MountedCount++] = pNode;
    return pNode;
}

Vnode *FsFindMountPoint(char Letter) {
    return pMountPoints[Letter - 'A'];
}

Vnode *FsFind(char *pPath) {
    char Letter = pPath[0];
    if (Letter > 'A') Letter = Letter - 'a' + 'A';
    Vnode *pMountPoint = FsFindMountPoint(Letter);
    char *pStr = MmAlloc(strlen(pPath) + 1);
    memcpy(pStr, pPath, strlen(pPath) + 1);
    char *pToken = strtok(pStr + 3, "/");
    Vnode *pCurrentNode = pMountPoint;
    while (pToken != NULL) {
        pCurrentNode = FsFindDir(pCurrentNode, pToken);
        if (pCurrentNode == NULL) {
            MmFree(pStr);
            return NULL;
        }
        pToken = strtok(NULL, "/");
    }
    MmFree(pStr);
    return pCurrentNode;
}

size_t FsRead(Vnode *pNode, uint8_t *pBuffer, size_t Length) {
    if (pNode->Read)
        return pNode->Read(pNode, pBuffer, Length);
    return 0;
}

size_t FsWrite(Vnode *pNode, uint8_t *pBuffer, size_t Length) {
    if (pNode->Write)
        return pNode->Write(pNode, pBuffer, Length);
    return 0;
}

DirEnt *FsReadDir(Vnode *pNode, uint32_t Index) {
    if (pNode->ReadDir)
        return pNode->ReadDir(pNode, Index);
    return NULL;
}

Vnode *FsFindDir(Vnode *pNode, char *pName) {
    if (pNode->FindDir)
        return pNode->FindDir(pNode, pName);
    return NULL;
}