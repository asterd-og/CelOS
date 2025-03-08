#include <ahci.h>
#include <printf.h>
#include <vmm.h>
#include <pmm.h>
#include <string.h>
#include <alloc.h>
#include <assert.h>

#if defined (__x86_64__)
#include <x86/pci.h>
#endif

#include <interrupt.h>

AhciPort *g_pAhciPorts[32];
int PortsConnected = 0;
int CommandSlots = 0;
HbaMem *pAbar;

int BlkAhciCheckType(HbaPort *pPort) {
    uint32_t SataStatus = pPort->SataStatus;
    uint8_t Ipm = (SataStatus >> 8) & 0x0F;
    uint8_t Det = SataStatus & 0x0F;
    if (Det != HBA_PORT_DET_PRESENT)
        return AHCI_DEV_NULL;
    if (Ipm != HBA_PORT_IPM_ACTIVE)
        return AHCI_DEV_NULL;
    switch (pPort->Signature) {
        case SATA_SIG_ATAPI:
            return AHCI_DEV_SATAPI;
        case SATA_SIG_SEMB:
            return AHCI_DEV_SEMB;
        case SATA_SIG_PM:
            return AHCI_DEV_PM;
        default:
            return AHCI_DEV_SATA;
    }
}

void BlkAhciStartCommand(HbaPort *pPort) {
    pPort->Command |= HBA_CMD_FRE;
    while (pPort->Command & HBA_CMD_CR)
        __asm__ volatile ("pause");
    pPort->Command |= HBA_CMD_ST;
}

void BlkAhciStopCommand(HbaPort *pPort) {
    pPort->Command &= ~HBA_CMD_ST;
    pPort->Command &= ~HBA_CMD_FRE;

    while ((pPort->Command & HBA_CMD_FR) || (pPort->Command & HBA_CMD_CR))
        __asm__ volatile ("pause");
}

AhciPort *BlkAhciPortRebase(HbaPort *pPort, int PortNum) {
    AhciPort *pAhciPort = (AhciPort*)MmAlloc(sizeof(AhciPort));
    pAhciPort->PortNum = PortNum;
    pAhciPort->pPort = pPort;

    BlkAhciStopCommand(pPort);

    void *pCommandBuffer = MmVirtAllocatePages(g_pKernelPageMap, 1, MM_READ | MM_WRITE);
    uint64_t CommandBufferPhys = MmGetPagePhysicalAddress(g_pKernelPageMap, (uint64_t)pCommandBuffer);
    pPort->LoCommandListAddr = (uint32_t)CommandBufferPhys;
    pPort->HiCommandListAddr = (uint32_t)(CommandBufferPhys >> 32);
    memset(pCommandBuffer, 0, PAGE_SIZE);
    pAhciPort->pCmdBuffer = pCommandBuffer;

    void *pFisBuffer = MmVirtAllocatePages(g_pKernelPageMap, 1, MM_READ | MM_WRITE);
    uint64_t FisBufferPhys = MmGetPagePhysicalAddress(g_pKernelPageMap, (uint64_t)pFisBuffer);
    pPort->LoFisAddr = (uint32_t)FisBufferPhys;
    pPort->HiFisAddr = (uint32_t)(FisBufferPhys >> 32);
    memset(pFisBuffer, 0, PAGE_SIZE);
    pAhciPort->pFisBuffer = pFisBuffer;

    HbaCmdHeader *pCmdHeader = (HbaCmdHeader*)pCommandBuffer;
    for (int i = 0; i < 32; i++) {
        pCmdHeader[i].PrdTableLen = 1;
        void *pCommandTable = MmVirtAllocatePages(g_pKernelPageMap, 1, MM_READ | MM_WRITE);
        uint64_t CmdTablePhys = MmGetPagePhysicalAddress(g_pKernelPageMap, (uint64_t)pCommandTable);
        pCmdHeader[i].LoCommandTableBaseAddr = (uint32_t)CmdTablePhys;
        pCmdHeader[i].HiCommandTableBaseAddr = (uint32_t)(CmdTablePhys >> 32);
        memset(pCommandTable, 0, PAGE_SIZE);
        pAhciPort->pCmdTables[i] = pCommandTable;
    }

    BlkAhciStartCommand(pPort);
    return pAhciPort;
}

int BlkAhciFindCmdSlot(HbaPort *pPort) {
    uint32_t Slots = (pPort->SataActive | pPort->CommandIssue);
    for (int i = 0; i < CommandSlots; i++) {
        if ((Slots & 1) == 0) return i;
        Slots >>= 1;
    }
    return -1;
}

void BlkAhciSendCmd(HbaPort *pPort, uint32_t Slot) {
    while (pPort->TaskFileData & 0x88)
        __asm__ volatile ("pause");

    pPort->Command &= ~HBA_CMD_ST;

    while (pPort->Command & HBA_CMD_CR)
        __asm__ volatile ("pause");

    pPort->Command |= (HBA_CMD_FR | HBA_CMD_ST);
    pPort->CommandIssue |= (1 << Slot);

    while (pPort->CommandIssue & (1 << Slot))
        __asm__ volatile ("pause");

    pPort->Command &= ~HBA_CMD_ST;
    while (pPort->Command & HBA_CMD_ST)
        __asm__ volatile ("pause");
    pPort->Command &= ~HBA_CMD_FRE;
}

int BlkAhciRead(AhciPort *pPort, uint64_t Lba, uint32_t Count, char *pBuffer) {
    HbaPort *pHbaPort = pPort->pPort;
    pHbaPort->InterruptStatus = (uint32_t)-1; // Clear pending interrupt bit.
    int Slot = BlkAhciFindCmdSlot(pPort->pPort);
    if (Slot == -1)
        return 1;

    HbaCmdHeader *pCmdHeader = (HbaCmdHeader*)pPort->pCmdBuffer;
    pCmdHeader += Slot;
    pCmdHeader->CmdFisLen = sizeof(FisRegH2D) / sizeof(uint32_t);
    pCmdHeader->Write = 0;
    pCmdHeader->PrdTableLen = (uint16_t)((Count - 1) >> 4) + 1;

    HbaCmdTable *pCmdTable = (HbaCmdTable*)pPort->pCmdTables[Slot];
    memset(pCmdTable, 0, sizeof(HbaCmdTable) + (pCmdHeader->PrdTableLen - 1) * sizeof(HbaPrdTableEntry));

    // TODO: Maybe start passing the physical address to the function
    // instead of the function getting the physical address
    // since if a user does an ahci read, it's gonna be on a different
    // page map from the kernel.

    uint64_t BufferPhys = MmGetPagePhysicalAddressOffset(g_pKernelPageMap, (uint64_t)pBuffer);

    int i = 0;
    for (; i < pCmdHeader->PrdTableLen - 1; i++) {
        pCmdTable->PrdTableEntries[i].LoDataBaseAddr = (uint32_t)BufferPhys;
        pCmdTable->PrdTableEntries[i].HiDataBaseAddr = (uint32_t)(BufferPhys >> 32);
        pCmdTable->PrdTableEntries[i].DataByteCount = 8 * 1024 - 1;
        pCmdTable->PrdTableEntries[i].Interrupt = 1;
        BufferPhys += 4 * 1024;
        Count -= 16;
    }
    pCmdTable->PrdTableEntries[i].LoDataBaseAddr = (uint32_t)BufferPhys;
    pCmdTable->PrdTableEntries[i].HiDataBaseAddr = (uint32_t)(BufferPhys >> 32);
    pCmdTable->PrdTableEntries[i].DataByteCount = Count * 512 - 1;
    pCmdTable->PrdTableEntries[i].Interrupt = 1;

    FisRegH2D *pFisCmd = (FisRegH2D*)(&pCmdTable->CommandFis);

    pFisCmd->FisType = FIS_TYPE_REG_H2D;
    pFisCmd->CommandSet = 1;
    pFisCmd->Command = 0x25; // Read DMA.

    pFisCmd->Lba0 = (uint8_t)Lba;
    pFisCmd->Lba1 = (uint8_t)(Lba >> 8);
    pFisCmd->Lba2 = (uint8_t)(Lba >> 16);
    pFisCmd->Lba3 = (uint8_t)(Lba >> 24);
    pFisCmd->Lba4 = (uint8_t)(Lba >> 32);
    pFisCmd->Lba5 = (uint8_t)(Lba >> 40);
    pFisCmd->Device = 64; // LBA Mode.

    pFisCmd->LoCount = (uint8_t)Count;
    pFisCmd->HiCount = (uint8_t)(Count >> 8);

    BlkAhciSendCmd(pHbaPort, Slot);

    if (pHbaPort->InterruptStatus & (1 << 30)) {
        printf("AHCI: Disk error on port %d.\n", pPort->PortNum);
        return 1;
    }

    return 0;
}

int BlkAhciInit() {
    PciHeader0 *pAhciDevice = IoPciGetDevice(1, 6);
    if (!pAhciDevice)
        return 1;
    // Enable Bus Mastering and memory space.
    pAhciDevice->Command |= (1 << 8) | (1 << 2) | (1 << 1);
    pAbar = MmVirtMapRange(g_pKernelPageMap, 0, pAhciDevice->Bars[5], 2, MM_READ | MM_WRITE);

    pAbar->GlobalHostControl = 1;
    while (pAbar->GlobalHostControl & 1)
        __asm__ volatile ("pause");

    // Enable AHCI.
    pAbar->GlobalHostControl |= (1 << 31);
    pAbar->GlobalHostControl &= ~(1 << 1);
    CommandSlots = (pAbar->Capabilities >> 8) & 0xF;
    pAbar->InterruptStatus = (uint32_t)-1;
    // Bitmap of connected ports.
    uint32_t PortImplemented = pAbar->PortImplemented;
    for (int i = 0; i < 32; i++) {
        if (PortImplemented & 1) {
            int DeviceType = BlkAhciCheckType(&pAbar->Ports[i]);
            if (DeviceType == AHCI_DEV_SATA) {
                printf("Found SATA disk at port %d.\n", i);
                g_pAhciPorts[PortsConnected++] = BlkAhciPortRebase(&pAbar->Ports[i], i);
                printf("Rebased and started port.\n");
            }
        }
        PortImplemented >>= 1;
    }
    printf("OK: AHCI Initialized.\n");
}