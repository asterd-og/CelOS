#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint16_t OffsetLow;
    uint16_t Selector;
    uint8_t Ist;
    uint8_t Flags;
    uint16_t OffsetMid;
    uint32_t OffsetHigh;
    uint32_t Reserved;
} __attribute__((packed)) IdtEntry;

typedef struct {
    uint16_t Size;
    uint64_t Offset;
} __attribute__((packed)) IdtDescriptor;

void KeIdtInit();
void KeIdtReload();