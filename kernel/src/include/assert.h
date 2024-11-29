#pragma once

#include <panic.h>

#define ASSERT(x) \
    do { \
        if(!(x)) { \
            PANIC("Assertion failed: " #x); \
        }\
    } while(0)
