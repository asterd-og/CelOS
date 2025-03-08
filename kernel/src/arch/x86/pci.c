#include <x86/pci.h>
#include <x86/ports.h>
#include <x86/acpi.h>
#include <pmm.h>
#include <printf.h>

#define PCI_CFG_ADDRESS 0xCF8
#define PCI_CFG_DATA 0xCFC

void *pDevices[256];
int DevicesIndex = 0;

uint16_t IoPciReadByte(uint8_t Bus, uint8_t Slot, uint8_t Func, uint8_t Offset) {
    return 0;
}

uint16_t IoPciReadWord(uint8_t Bus, uint8_t Slot, uint8_t Func, uint8_t Offset) {
    return 0;
}

uint32_t IoPciReadDword(uint32_t Bus, uint32_t Slot, uint32_t Func, uint16_t Offset) {
    return 0;
}

uint8_t IoPcieReadByte(McfgEntry *pEntry, uint32_t Bus, uint32_t Slot, uint32_t Func, uint32_t Offset) {
    uint64_t PhysicalAddr = pEntry->BaseAddress + (((Bus - pEntry->StartPciBus) << 20) | (Slot << 15) | (Func << 12)) + Offset;
    return *(uint8_t*)HIGHER_HALF(PhysicalAddr);
}

uint16_t IoPcieReadWord(McfgEntry *pEntry, uint32_t Bus, uint32_t Slot, uint32_t Func, uint32_t Offset) {
    uint64_t PhysicalAddr = pEntry->BaseAddress + (((Bus - pEntry->StartPciBus) << 20) | (Slot << 15) | (Func << 12)) + Offset;
    return *(uint16_t*)HIGHER_HALF(PhysicalAddr);
}

uint32_t IoPcieReadDword(McfgEntry *pEntry, uint32_t Bus, uint32_t Slot, uint32_t Func, uint32_t Offset) {
    uint64_t PhysicalAddr = pEntry->BaseAddress + (((Bus - pEntry->StartPciBus) << 20) | (Slot << 15) | (Func << 12)) + Offset;
    return *(uint32_t*)HIGHER_HALF(PhysicalAddr);
}

void IoPciLegacyInit() {
    uint16_t Vendor;
    uint16_t Device;
    uint8_t Class;
    uint8_t SubClass;

    // TODO:
    for (uint8_t Bus = 0; Bus < 255; Bus++) {
        for (uint8_t Slot = 0; Slot < 32; Slot++) {
            for (uint8_t Func = 0; Func < 8; Func++) {
            }
        }
    }
}

void IoPciInit() {
    McfgDescriptor *pMcfg = KeAcpiFindSdt("MCFG");
    if (pMcfg == NULL) {
        printf("Could not Find MCFG. Using legacy PCI.\n");
        return IoPciLegacyInit();
    }

    McfgEntry *pEntry = &pMcfg->Table[0];
    uint64_t EntryCount = (pMcfg->Length - sizeof(McfgDescriptor)) / sizeof(McfgEntry);

    for (uint64_t i = 0; i < EntryCount; i++) {
        pEntry = &pMcfg->Table[i];
        for (uint8_t Bus = pEntry->StartPciBus; Bus < pEntry->EndPciBus; Bus++) {
            for (uint8_t Slot = 0; Slot < 32; Slot++) {
                for (uint8_t Func = 0; Func < 8; Func++) {
                    uint64_t PhysicalAddr = pEntry->BaseAddress + (((Bus - pEntry->StartPciBus) << 20) | (Slot << 15) | (Func << 12));
                    PciHeader0 *pDevice = (PciHeader0*)HIGHER_HALF(PhysicalAddr);
                    if (pDevice->Vendor == 0xFFFF) continue;
                    printf("Found PCI Device Vendor: %x, Device: %x, Class: %d, Sub Class: %d.\n",
                       pDevice->Vendor, pDevice->Device, pDevice->Class, pDevice->SubClass);
                    pDevices[DevicesIndex++] = pDevice;
                }
            }
        }
    }
}

void *IoPciGetDevice(uint8_t Class, uint8_t SubClass) {
    for (int i = 0; i < DevicesIndex; i++) {
        PciHeader0 *pDevice = pDevices[i];
        if (pDevice->Class == Class && pDevice->SubClass == SubClass) {
            return pDevice;
        }
    }
    return NULL;
}