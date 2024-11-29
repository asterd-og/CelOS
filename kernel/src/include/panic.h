#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

void KePanic(const char *pszFile, const int line, const char *pszFmt, ...);

#define PANIC(msg, ...) KePanic(__FILE__, __LINE__, msg, ##__VA_ARGS__)