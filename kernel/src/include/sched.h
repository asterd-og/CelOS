#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <context.h>
#include <alloc.h>
#include <vmm.h>
#include <list.h>

#define TASK_READY 1
#define TASK_RUNNING 2
#define TASK_SLEEPING 4
#define TASK_BLOCKED 8
#define TASK_ZOMBIE 16

enum TaskPriority {
    TASK_LOW = 1,
    TASK_MED = 2,
    TASK_HIGH = 3
};

typedef struct Task {
    uint64_t ID;
    Context Ctx;
    PageMap *pPageMap;
    uint64_t Stack;
    uint64_t Priority;
    uint64_t Flags;
    uint32_t CpuNum;
} Task;

extern bool g_SchedInitialised;

void KxSchedInit();

Task *KxCreateTask(void *pEntry, uint64_t Priority, uint32_t CpuNum);

void KxKillTask();
void KxBlockTask();

void KxBlockSched();
void KxUnblockSched();
