// Advanced terminal with ssfn support

#define SSFN_IMPLEMENTATION
#include <advterm.h>
#include <ssfn.h>
#include <kernel.h>
#include <spinlock.h>

ssfn_t SfnCtx = { 0 };
ssfn_buf_t SfnBuf;

uint32_t CursorX = 0;
uint32_t CursorY = 0;
uint32_t CursorWidth = 0;
uint32_t CursorHeight = 0;
uint32_t Foreground = 0xFFFFFFFF;
uint32_t Background = 0x0;

void KeTermInit(uint32_t* pFramebufferAddress, uint32_t FramebufferWidth, uint32_t FramebufferHeight, uint32_t FramebufferPitch, uint32_t Background, uint32_t Foreground, uint32_t CursorColor, void *pFontAddress) {
    SfnBuf.ptr = pFramebufferAddress;
    SfnBuf.w = FramebufferWidth;
    SfnBuf.h = FramebufferHeight;
    SfnBuf.p = FramebufferPitch;
    SfnBuf.x = 0;
    SfnBuf.y = 24;
    SfnBuf.fg = 0xFFFFFFFF;
    ssfn_load(&SfnCtx, pFontAddress);
    ssfn_select(&SfnCtx, SSFN_FAMILY_ANY, NULL, SSFN_STYLE_REGULAR, 24);
    memset(pFramebufferAddress, 0, FramebufferWidth * FramebufferHeight * 4);
    PutCharCallBack = KeTermWriteChar;

    int Left = 0;
    int Top = 0;
    char String[2] = {'A', 0};
    ssfn_bbox(&SfnCtx, String, &CursorWidth, &CursorHeight, &Left, &Top);
}

SpinLock TermSpinLock;

void KeTermFillRect(ssfn_buf_t *pBuf, uint32_t x, uint32_t y, uint32_t Width, uint32_t Height, uint32_t Color) {
    uint32_t *pBuffer = (uint32_t*)pBuf->ptr;
    for (uint32_t y1 = 0; y1 < Height; y1++) {
        for (uint32_t x1 = 0; x1 < Width; x1++) {
            pBuffer[(y1 + y) * pBuf->p / 4 + (x1 + x)] = Color;
        }
    }
}

uint32_t Palette[] = {
    0,
    0xFFFF0000,
    0xFF00FF00,
    0xFFFFFF00,
    0xFF0000FF,
    0xFFFF00FF,
    0xFF00FFFF,
    0xFFFFFFFF,
    0xFFFFFFFF
};

char AnsiBuffer[32];
int AnsiBufferIndex = 0;
bool HandlingAnsi = false;

int KeTermHandleAnsi(char Char) {
    if (Char == 'm') {
        // Todo: Add more ansi escape sequences
        AnsiBuffer[AnsiBufferIndex] = 'm';
        AnsiBufferIndex = (AnsiBuffer[0] == '[' ? 1 : 0);
        while (HandlingAnsi) {
            switch (AnsiBuffer[AnsiBufferIndex]) {
                case '3':
                    AnsiBufferIndex++;
                    Foreground = Palette[AnsiBuffer[AnsiBufferIndex] - '0'];
                    SfnBuf.fg = Foreground;
                    break;
                case '4':
                    AnsiBufferIndex++;
                    Background = Palette[AnsiBuffer[AnsiBufferIndex] - '0'];
                    break;
                case ';':
                    break;
                case 'm':
                    HandlingAnsi = false;
                    break;
                default:
                    E9Write("Unexpected ansi character %d.\n\n\n", AnsiBuffer[AnsiBufferIndex]);
                    HandlingAnsi = false;
                    AnsiBufferIndex = 0;
                    break;
            }
            AnsiBufferIndex++;
        }
        AnsiBufferIndex = 0;
        return 0;
    }
    AnsiBuffer[AnsiBufferIndex++] = Char;
    return 0;
}

void KeTermWriteChar(void *pUnused, char Char) {
    if (Char == 0) return;
    if (Char == '\x1b') {
        HandlingAnsi = true;
        return;
    }
    if (HandlingAnsi) {
        KeTermHandleAnsi(Char);
        return;
    }
    (void)pUnused;
    SpinLockAcquire(&TermSpinLock);

    KeTermFillRect(&SfnBuf, CursorX, CursorY, CursorWidth, CursorHeight, 0);
    if (Char == '\n') {
        SfnBuf.y += SfnCtx.size;
        SfnBuf.x = 0;
        CursorX = 0;
        CursorY += CursorHeight;
        KeTermFillRect(&SfnBuf, CursorX, CursorY + CursorHeight - 4, CursorWidth, 4, 0xFFFFFFFF);
        SpinLockRelease(&TermSpinLock);
        return;
    }
    int Width = 0;
    int Height = 0;
    int Left = 0;
    int Top = 0;

    char String[] = {Char, 0};
    int Status = ssfn_render(&SfnCtx, &SfnBuf, &Char);
    ssfn_bbox(&SfnCtx, String, &Width, &Height, &Left, &Top);
    CursorX += Width;
    KeTermFillRect(&SfnBuf, CursorX, CursorY + Height - 4, CursorWidth, 4, 0xFFFFFFFF);

    SpinLockRelease(&TermSpinLock);
}