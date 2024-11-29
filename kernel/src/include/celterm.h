#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t CursorX;
    uint32_t CursorY;
    uint32_t CursorWidth;
    uint32_t CursorHeight;
    uint32_t CursorColor;

    uint32_t Foreground;
    uint32_t Background;

    uint32_t FontWidth;
    uint32_t FontHeight;
    uint8_t *pFontData;

    uint32_t *pFramebufferAddress;
    uint32_t FramebufferWidth;
    uint32_t FramebufferHeight;
    uint32_t FramebufferPitch;
} TermCtrl;

TermCtrl *TeNew(uint32_t* pFramebufferAddress, uint32_t FramebufferWidth, uint32_t FramebufferHeight, uint32_t FramebufferPitch, uint32_t Background, uint32_t Foreground, uint32_t CursorColor);
void TeWriteChar(TermCtrl *pTermCtrl, char Char);