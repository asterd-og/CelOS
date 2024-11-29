#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t Entries[5];
} __attribute__((packed)) GdtTable;

typedef struct {
    uint16_t Size;
    uint64_t Address;
} __attribute__((packed)) GdtDescriptor;

void KeGdtInit();
