#include <x86/madt.h>
#include <x86/acpi.h>
#include <printf.h>

uint32_t *g_pLocalApicPhysAddr = NULL;

MadtIoApic *g_pIoApicList[128];

MadtIoApicIntSrcOvr *g_pIoApicIntSrcOvrList[128];
uint64_t g_IoApicIntSrcOvrSize = 0;

MadtLocalApicNmi *g_pLocalApicNmiList[128];
uint64_t g_LocalApicNmiSize = 0;

MadtLocalX2Apic *g_pLocalX2ApicList[128];
uint64_t g_LocalX2ApicSize = 0;

void KeMadtInit() {
    MadtDescriptor *pMadtDesc = KeAcpiFindSdt("APIC");
    uint64_t Offset = 0;
    uint64_t IoApicListIndex = 0;
    while (Offset < pMadtDesc->Length - sizeof(MadtDescriptor)) {
        MadtEntry *pEntry = (MadtEntry*)(pMadtDesc->aTable + Offset);
        switch (pEntry->Type) {
            case 1: { // IO APIC
                MadtIoApic *pMadtIoApic = (MadtIoApic*)pEntry;
                g_pIoApicList[IoApicListIndex++] = pMadtIoApic;
                break;
            }
            case 2: { // IO APIC Interrupt Source Override
                MadtIoApicIntSrcOvr *pIoApicIntSrcOvr = (MadtIoApicIntSrcOvr*)pEntry;
                g_pIoApicIntSrcOvrList[g_IoApicIntSrcOvrSize++] = pIoApicIntSrcOvr;
                break;
            }
            case 4: { // Local APIC NMI
                MadtLocalApicNmi *pLocalApicNmi = (MadtLocalApicNmi*)pEntry;
                g_pLocalApicNmiList[g_LocalApicNmiSize++] = pLocalApicNmi;
                break;
            }
            case 5: { // Local APIC Address Override
                MadtLocalApicAddr *pMadtLocalApicAddr = (MadtLocalApicAddr*)pEntry;
                g_pLocalApicPhysAddr = (uint32_t*)pMadtLocalApicAddr->LocalApicPhysAddr;
                break;
            }
            case 9: { // Processor Local x2APIC
                MadtLocalX2Apic *pLocalX2Apic = (MadtLocalX2Apic*)pEntry;
                g_pLocalX2ApicList[g_LocalX2ApicSize++] = pLocalX2Apic;
                break;
            }
        }
        Offset += pEntry->Length;
    }
    if (g_pLocalApicPhysAddr == NULL)
        g_pLocalApicPhysAddr = (uint32_t*)pMadtDesc->LocalApicAddress;

}