#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    char aSignature[8];
    uint8_t Checksum;
    char aId[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
} __attribute__((packed)) RsdpDescriptor;

typedef struct {
    char aSignature[8];
    uint8_t Checksum;
    char aId[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t aReserved[3];
} __attribute__((packed)) XsdpDescriptor;

typedef struct {
    char aSignature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char aId[6];
    char aTableId[8];
    uint32_t OemRevision;
    uint32_t CreatorId;
    uint32_t CreatorRevision;
} __attribute__((packed)) SdtHeader;

typedef struct {
    SdtHeader Sdt;
    char aTable[];
} SdtDescriptor;

void KeAcpiInit();
void *KeAcpiFindSdt(char *pSignature);