#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct ListItem {
    void *pData;
    struct ListItem *pNext;
    struct ListItem *pPrev;
} ListItem;

typedef struct {
    ListItem *pHead;
    size_t Count;
} List;

List *ListCreate();
void ListAppend(List *pList, void *pData);
