#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <context.h>

#define INT_OK 0
#define INT_UNCAUGHT 1
#define INT_QUEUED 2

typedef struct {
    bool Present;
    void (*pHandler)(Context*);
    uint8_t IPL;
} IrqDescriptor;

int KxHandleIrq(uint8_t Irq, Context *pCtx);
uint8_t KxGetFreeIrq();
void KxInstallIrq(uint8_t Irq, void *pHandler, uint8_t IPL);
void KxUninstallIrq(uint8_t Irq);
void KxSendInt(uint32_t CpuNum, uint8_t Irq);
void KxLowerIpl(uint8_t IPL);