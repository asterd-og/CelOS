#include <kernel.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#if defined (__x86_64__)
#include <x86/idt.h>
#include <x86/apic.h>
#include <x86/e9.h>
#include <x86/pci.h>
#endif

#include <smp.h>
#include <arch.h>
#include <sched.h>

#include <pmm.h>
#include <vmm.h>
#include <alloc.h>

#include <celterm.h>
#include <printf.h>
#include <string.h>
#include <panic.h>
#include <assert.h>

#include <ahci.h>
#include <vfs.h>
#include <bfs.h>
#include <diskfs.h>

#include <advterm.h>

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request FramebufferRequest = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request HhdmRequest = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request ModuleRequest = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}

TermCtrl *pTermCtrl;
void (*PutCharCallBack)(void *, char) = TeWriteChar;

void _putchar(char Char) {
    PutCharCallBack(pTermCtrl, Char);
}

uint64_t HhdmOffset;

void KrnlTask();

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
void KeMain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (FramebufferRequest.response == NULL
     || FramebufferRequest.response->framebuffer_count < 1) {
        hcf();
    }

    HhdmOffset = HhdmRequest.response->offset;

    struct limine_framebuffer *pFramebuffer = FramebufferRequest.response->framebuffers[0];

    pTermCtrl = TeNew(pFramebuffer->address,
                      pFramebuffer->width, pFramebuffer->height,
                      pFramebuffer->pitch,
                      0xff000000,
                      0xffffffff, 0xfff39b00);

    KeArchInit();
    printf("All CPUs initialised.\n");

    printf("Kernel Initialised.\n");

    IoPciInit();
    BlkAhciInit();

    ASSERT(DiskBfsMount() == 0);

    Vnode *pFont = FsFind("a:/sfmono.fnt");
    uint8_t *pFontBuffer = (uint8_t*)MmAlloc(pFont->Size);
    FsRead(pFont, pFontBuffer, pFont->Size);

    KeTermInit(pFramebuffer->address,
                      pFramebuffer->width, pFramebuffer->height,
                      pFramebuffer->pitch,
                      0xff000000,
                      0xffffffff, 0xfff39b00, pFontBuffer);

    printf("\x1b[33mCelOS\x1b[38m booted successfully.\n");

    KxCreateTask(KrnlTask, TASK_HIGH, 0);
    KxSchedInit();

    // We're done, just hang...
    hcf();
}

void KrnlTask() {
    for (;;);
}
