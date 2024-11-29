#pragma once

#include <stdint.h>
#include <stddef.h>

#if defined (__x86_64__)

typedef struct {
    uint64_t R15;
    uint64_t R14;
    uint64_t R13;
    uint64_t R12;
    uint64_t R11;
    uint64_t R10;
    uint64_t R9;
    uint64_t R8;
    uint64_t RDI;
    uint64_t RSI;
    uint64_t RBP;
    uint64_t RBX;
    uint64_t RDX;
    uint64_t RCX;
    uint64_t RAX;
    uint64_t IntNo;
    uint64_t ErrorCode;
    uint64_t RIP;
    uint64_t CS;
    uint64_t RFlags;
    uint64_t RSP;
    uint64_t SS;
} __attribute__((packed)) Context;

#define CTX_STK(ctx) ctx.RSP
#define CTX_IP(ctx) ctx.RIP

#endif
