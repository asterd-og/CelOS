#include <panic.h>
#include <printf.h>

void KePanic(const char *pszFile, const int Line, const char *pszFmt, ...) {
    va_list VaList;
    va_start(VaList, pszFmt);
    char szBuffer[1024];
    vsnprintf(szBuffer, (size_t)-1, pszFmt, VaList);
    va_end(VaList);
    printf("PANIC @ %s:%d: %s", pszFile, Line, szBuffer);
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__riscv)
        asm ("wfi");
#endif
    }
}
