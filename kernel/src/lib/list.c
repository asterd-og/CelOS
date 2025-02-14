#include <list.h>
#include <alloc.h>

List *ListCreate() {
    List *pList = (List*)MmAlloc(sizeof(List));
    pList->pHead = (ListItem*)MmAlloc(sizeof(ListItem));
    pList->pHead->pNext = pList->pHead->pPrev = pList->pHead;
    pList->Count = 0;
    return pList;
}

void ListAppend(List *pList, void *pData) {
    ListItem *pItem = (ListItem*)MmAlloc(sizeof(ListItem));
    pItem->pData = pData;
    pItem->pNext = pList->pHead;
    pItem->pPrev = pList->pHead->pPrev;
    pList->pHead->pPrev->pNext = pItem;
    pList->pHead->pPrev = pItem;
    pList->Count++;
}
