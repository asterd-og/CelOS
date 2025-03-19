#pragma once

#include <stdint.h>
#include <stddef.h>

void KeTermInit(uint32_t* pFramebufferAddress, uint32_t FramebufferWidth, uint32_t FramebufferHeight, uint32_t FramebufferPitch, uint32_t Background, uint32_t Foreground, uint32_t CursorColor, void *pFontAddress);
void KeTermWriteChar(void *pUnused, char Char);