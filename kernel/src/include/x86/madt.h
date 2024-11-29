#pragma once

#include <stdint.h>
#include <stddef.h>

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
    // MADT
    uint32_t LocalApicAddress;
    uint32_t Flags;

    char aTable[];
} MadtDescriptor;

typedef struct {
    uint8_t Type;
    uint8_t Length;
} MadtEntry;

typedef struct {
    MadtEntry Entry;
    uint8_t IoApicId;
    uint8_t Reserved;
    uint32_t IoApicPhysAddress;
    uint32_t GsiBase;
} MadtIoApic;

typedef struct {
    MadtEntry Entry;
    uint8_t BusSource;
    uint8_t IrqSource;
    uint32_t Gsi;
    uint16_t Flags;
} MadtIoApicIntSrcOvr;

typedef struct {
    MadtEntry Entry;
    uint8_t CpuID;
    uint16_t Flags;
    uint8_t LintNum;
} MadtLocalApicNmi;

typedef struct {
    MadtEntry Entry;
    uint16_t Reserved;
    uint64_t LocalApicPhysAddr;
} MadtLocalApicAddr;

typedef struct {
    MadtEntry Entry;
    uint32_t LocalX2ApicID;
    uint32_t Flags;
    uint32_t CpuID;
} MadtLocalX2Apic;

extern uint32_t *g_pLocalApicPhysAddr;
extern MadtIoApic *g_pIoApicList[128];
extern MadtIoApicIntSrcOvr *g_pIoApicIntSrcOvrList[128];
extern uint64_t g_IoApicIntSrcOvrSize;

extern MadtLocalApicNmi *g_pLocalApicNmiList[128];
extern uint64_t g_LocalApicNmiSize;

extern MadtLocalX2Apic *g_pLocalX2ApicList[128];
extern uint64_t g_LocalX2ApicSize;

void KeMadtInit();