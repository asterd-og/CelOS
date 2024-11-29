#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef bool SpinLock;

void SpinLockAcquire(SpinLock *pSpinLock);
void SpinLockRelease(SpinLock *pSpinLock);
