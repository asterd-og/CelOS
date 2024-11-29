#include <serial.h>
#include <printf.h>
#include <x86/ports.h>
#include <spinlock.h>

SpinLock g_E9Lock;

void E9Write(const char *pszFmt, ...) {
    SpinLockAcquire(&g_E9Lock);
    va_list VaList;
    va_start(VaList, pszFmt);
    char szBuffer[1024];
    vsnprintf(szBuffer, (size_t)-1, pszFmt, VaList);
    va_end(VaList);
    int i = 0;
    while (szBuffer[i]) {
        KeWritePort8(0xe9, szBuffer[i]);
        i++;
    }
    SpinLockRelease(&g_E9Lock);
}
