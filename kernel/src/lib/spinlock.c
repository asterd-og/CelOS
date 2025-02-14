#include <spinlock.h>

void SpinLockAcquire(SpinLock *pSpinLock) {
    while (__atomic_test_and_set(pSpinLock, __ATOMIC_ACQUIRE))
        __asm__ volatile ("pause");
}

void SpinLockRelease(SpinLock *pSpinLock) {
    __atomic_clear(pSpinLock, __ATOMIC_RELEASE);
}
