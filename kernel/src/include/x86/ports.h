#pragma once

#include <stdint.h>

inline void KeWritePort8(uint16_t Port, uint8_t Data) {
    __asm__ volatile ("outb %0, %1" : : "a"(Data), "Nd"(Port) : "memory");
}

inline uint8_t KeReadPort8(uint16_t Port) {
    uint8_t Ret;
    __asm__ volatile ("inb %1, %0" : "=a"(Ret) : "Nd"(Port) : "memory");
    return Ret;
}

inline void KeWritePort16(uint16_t Port, uint16_t Data) {
    __asm__ volatile ("outw %0, %1" : : "a"(Data), "Nd"(Port) : "memory");
}

inline uint16_t KeReadPort16(uint16_t Port) {
    uint16_t Ret;
    __asm__ volatile ("inw %1, %0" : "=a"(Ret) : "Nd"(Port) : "memory");
    return Ret;
}

inline void KeWritePort32(uint16_t Port, uint32_t Data) {
    __asm__ volatile ("outl %0, %1" : : "a"(Data), "Nd"(Port) : "memory");
}

inline uint32_t KeReadPort32(uint16_t Port) {
    uint32_t Ret;
    __asm__ volatile ("inl %1, %0" : "=a"(Ret) : "Nd"(Port) : "memory");
    return Ret;
}