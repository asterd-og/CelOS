#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t BaseAddress;
    uint16_t PciSegment;
    uint8_t StartPciBus;
    uint8_t EndPciBus;
    uint32_t Reserved;
} __attribute__((packed)) McfgEntry;

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

    uint64_t Reserved;
    McfgEntry Table[];
} __attribute__((packed)) McfgDescriptor;

typedef struct {
    uint16_t Vendor;
    uint16_t Device;
    uint16_t Command;
    uint16_t Status;
    uint8_t Revision;
    uint8_t ProgIf;
    uint8_t SubClass;
    uint8_t Class;
    uint8_t CacheLineSize;
    uint8_t LatencyTimer;
    uint8_t HeaderType;
    uint8_t Bist;
    uint32_t Bars[6];
    uint32_t CardBus;
    uint16_t SubSystemVendorId;
    uint16_t SubSystemId;
    uint32_t ExpansionRomBaseAddr;
    uint8_t Capabilities;
    uint32_t Reserved : 24;
    uint32_t Reserved1;
    uint8_t InterruptLine;
    uint8_t InterruptPin;
    uint8_t MinGrant;
    uint8_t MaxLatency;
} __attribute__((packed)) PciHeader0;

void IoPciInit();
void *IoPciGetDevice(uint8_t Class, uint8_t SubClass);
