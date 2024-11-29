[global KeGdtReloadSeg]

KeGdtReloadSeg:
    push 0x08
    lea RAX, [rel .KeGdtReloadCS]
    push RAX
    retfq
.KeGdtReloadCS:
    mov AX, 0x10
    mov DS, AX
    mov ES, AX
    mov FS, AX
    mov GS, AX
    mov SS, AX
    ret