#include <x86/acpi.h>
#include <stdbool.h>
#include <string.h>
#include <limine.h>
#include <assert.h>
#include <pmm.h>

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request RsdpRequest = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

void *pSdtAddress = NULL;
bool UseXsdt = false;

void KeAcpiInit() {
    struct limine_rsdp_response *pRsdpResponse = RsdpRequest.response;
    RsdpDescriptor *pRsdpDesc = (RsdpDescriptor*)HIGHER_HALF(pRsdpResponse->address);
    ASSERT(!memcmp(pRsdpDesc->aSignature, "RSD PTR ", 8));
    UseXsdt = (pRsdpDesc->Revision == 2);
    if (UseXsdt) {
        XsdpDescriptor *pXsdpDesc = (XsdpDescriptor*)HIGHER_HALF(pRsdpResponse->address);
        pSdtAddress = (void*)HIGHER_HALF(pXsdpDesc->XsdtAddress);
    } else {
        pSdtAddress = (void*)HIGHER_HALF(pRsdpDesc->RsdtAddress);
    }
    ASSERT(pSdtAddress);
}

void *KeAcpiFindSdt(char *pSignature) {
    SdtDescriptor *pSdtDesc = (SdtDescriptor*)pSdtAddress;
    uint32_t EntrySize = 4;
    if (UseXsdt)
        EntrySize = 8;
    uint32_t Entries = (pSdtDesc->Sdt.Length - sizeof(pSdtDesc->Sdt)) / EntrySize;
    for (uint32_t i = 0; i < Entries; i++) {
        SdtHeader *pSdt;
        if (UseXsdt)
            pSdt = (SdtHeader*)HIGHER_HALF(*((uint64_t*)pSdtDesc->aTable + i));
        else
            pSdt = (SdtHeader*)HIGHER_HALF(*((uint32_t*)pSdtDesc->aTable + i));
        if (!memcmp(pSdt->aSignature, pSignature, 4))
            return (void*)pSdt;
    }
    return NULL;
}