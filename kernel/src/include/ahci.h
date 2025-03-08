#pragma once

#include <stdint.h>
#include <stddef.h>

#define	SATA_SIG_ATA   0x00000101
#define	SATA_SIG_ATAPI 0xEB140101
#define	SATA_SIG_SEMB  0xC33C0101
#define	SATA_SIG_PM    0x96690101

#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define AHCI_DEV_SATAPI 4

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

#define HBA_CMD_ST    0x0001
#define HBA_CMD_FRE   0x0010
#define HBA_CMD_FR    0x4000
#define HBA_CMD_CR    0x8000

#define FIS_TYPE_REG_H2D 0x27

typedef struct {
    uint32_t LoCommandListAddr;
    uint32_t HiCommandListAddr;
    uint32_t LoFisAddr;
    uint32_t HiFisAddr;
    uint32_t InterruptStatus;
    uint32_t InterruptEnable;
    uint32_t Command;
    uint32_t Reserved;
    uint32_t TaskFileData;
    uint32_t Signature;
    uint32_t SataStatus;
    uint32_t SataControl;
    uint32_t SataError;
    uint32_t SataActive;
    uint32_t CommandIssue;
    uint32_t SataNotification;
    uint32_t FisSwitchControl;
    uint32_t Reserved2[11];
    uint32_t Vendor[4];
} HbaPort;

typedef struct {
    uint32_t Capabilities;
    uint32_t GlobalHostControl;
    uint32_t InterruptStatus;
    uint32_t PortImplemented;
    uint32_t Version;
    uint32_t CccCtl;
    uint32_t CccPts;
    uint32_t EmLoc;
    uint32_t EmCtl;
    uint32_t Capabilities2;
    uint32_t Bohc;
    uint8_t Reserved[116];
    uint8_t Vendor[96];
    HbaPort Ports[];
} HbaMem;

typedef struct {
    uint8_t CmdFisLen : 5; // In dwords.
    uint8_t AtaPi : 1;
    uint8_t Write : 1;
    uint8_t Prefetchable : 1;

    uint8_t Reset : 1;
    uint8_t Bist : 1;
    uint8_t Clear : 1; // Clear busy upon R_OK.
    uint8_t Reserved : 1;
    uint8_t PortMultiplierPort : 4;

    uint16_t PrdTableLen; // Physical region description table length in entries.

    volatile uint32_t PrdByteCount; // Physical region descriptor byte count transferred.

    uint32_t LoCommandTableBaseAddr;
    uint32_t HiCommandTableBaseAddr;

    uint32_t Reserved1[4];
} __attribute__((packed)) HbaCmdHeader;

typedef struct {
    uint32_t LoDataBaseAddr;
    uint32_t HiDataBaseAddr;
    uint32_t Reserved;

    uint32_t DataByteCount : 22;
    uint32_t Reserved1 : 9;
    uint32_t Interrupt : 1;
} __attribute__((packed)) HbaPrdTableEntry;

typedef struct {
    uint8_t CommandFis[64];

    uint8_t AtaPiCmd[16];

    uint8_t Reserved[48];

    HbaPrdTableEntry PrdTableEntries[];
} HbaCmdTable;

typedef struct {
    uint8_t FisType;

    uint8_t PortMultiplierPort : 4;
    uint8_t Reserved : 3;
    uint8_t CommandSet : 1; // 1: Command, 0: Control.

    uint8_t Command;
    uint8_t LoFeature;

    uint8_t Lba0;
    uint8_t Lba1;
    uint8_t Lba2;
    uint8_t Device;

    uint8_t Lba3;
    uint8_t Lba4;
    uint8_t Lba5;
    uint8_t HiFeature;

    uint8_t LoCount;
    uint8_t HiCount;
    uint8_t Icc;
    uint8_t Control;

    uint32_t Reserved1;
} __attribute__((packed)) FisRegH2D;

typedef struct {
    int PortNum;
    HbaPort *pPort;
    void *pCmdBuffer;
    void *pFisBuffer;
    void *pCmdTables[32];
} AhciPort;

extern AhciPort *g_pAhciPorts[32];

int BlkAhciInit();
int BlkAhciRead(AhciPort *pPort, uint64_t Lba, uint32_t Count, char *pBuffer);