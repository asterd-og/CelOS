[extern KeHandleIsr]
[extern KeHandleIrq]
[global g_pIdtIntTable]

%macro Pushaq 0
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro Popaq 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
%endmacro

%macro IsrNoErrStub 1
IntStub%+%1:
    push 0
    push %1
    Pushaq
    
    mov RDI, RSP
    call KeHandleIsr

    Popaq
    add RSP, 16 ; Pop 0 and %1
    iretq
%endmacro

%macro IsrErrStub 1
IntStub%+%1:
    push %1
    Pushaq
    
    mov RDI, RSP
    call KeHandleIsr

    Popaq
    add RSP, 16 ; Pop %1 and error code (Gets pushed automatically by the CPU
    iretq
%endmacro

%macro IrqStub 1
IntStub%+%1:
    push 0
    push %1
    Pushaq
    
    mov RDI, RSP
    call KeHandleIrq

    Popaq
    add RSP, 16 ; Pop 0 and %1
    iretq
%endmacro

IsrNoErrStub 0
IsrNoErrStub 1
IsrNoErrStub 2
IsrNoErrStub 3
IsrNoErrStub 4
IsrNoErrStub 5
IsrNoErrStub 6
IsrNoErrStub 7
IsrErrStub 8
IsrNoErrStub 9
IsrErrStub 10
IsrErrStub 11
IsrErrStub 12
IsrErrStub 13
IsrErrStub 14
IsrNoErrStub 15
IsrNoErrStub 16
IsrErrStub 17
IsrNoErrStub 18
IsrNoErrStub 19
IsrNoErrStub 20
IsrNoErrStub 21
IsrNoErrStub 22
IsrNoErrStub 23
IsrNoErrStub 24
IsrNoErrStub 25
IsrNoErrStub 26
IsrNoErrStub 27
IsrNoErrStub 28
IsrNoErrStub 29
IsrErrStub 30
IsrNoErrStub 31

IrqStub 32
IrqStub 33
IrqStub 34
IrqStub 35
IrqStub 36
IrqStub 37
IrqStub 38
IrqStub 39
IrqStub 40
IrqStub 41
IrqStub 42
IrqStub 43
IrqStub 44
IrqStub 45
IrqStub 46
IrqStub 47
IrqStub 48
IrqStub 49
IrqStub 50
IrqStub 51
IrqStub 52
IrqStub 53
IrqStub 54
IrqStub 55
IrqStub 56
IrqStub 57
IrqStub 58
IrqStub 59
IrqStub 60
IrqStub 61
IrqStub 62
IrqStub 63
IrqStub 64
IrqStub 65
IrqStub 66
IrqStub 67
IrqStub 68
IrqStub 69
IrqStub 70
IrqStub 71
IrqStub 72
IrqStub 73
IrqStub 74
IrqStub 75
IrqStub 76
IrqStub 77
IrqStub 78
IrqStub 79
IrqStub 80
IrqStub 81
IrqStub 82
IrqStub 83
IrqStub 84
IrqStub 85
IrqStub 86
IrqStub 87
IrqStub 88
IrqStub 89
IrqStub 90
IrqStub 91
IrqStub 92
IrqStub 93
IrqStub 94
IrqStub 95
IrqStub 96
IrqStub 97
IrqStub 98
IrqStub 99
IrqStub 100
IrqStub 101
IrqStub 102
IrqStub 103
IrqStub 104
IrqStub 105
IrqStub 106
IrqStub 107
IrqStub 108
IrqStub 109
IrqStub 110
IrqStub 111
IrqStub 112
IrqStub 113
IrqStub 114
IrqStub 115
IrqStub 116
IrqStub 117
IrqStub 118
IrqStub 119
IrqStub 120
IrqStub 121
IrqStub 122
IrqStub 123
IrqStub 124
IrqStub 125
IrqStub 126
IrqStub 127
IrqStub 128
IrqStub 129
IrqStub 130
IrqStub 131
IrqStub 132
IrqStub 133
IrqStub 134
IrqStub 135
IrqStub 136
IrqStub 137
IrqStub 138
IrqStub 139
IrqStub 140
IrqStub 141
IrqStub 142
IrqStub 143
IrqStub 144
IrqStub 145
IrqStub 146
IrqStub 147
IrqStub 148
IrqStub 149
IrqStub 150
IrqStub 151
IrqStub 152
IrqStub 153
IrqStub 154
IrqStub 155
IrqStub 156
IrqStub 157
IrqStub 158
IrqStub 159
IrqStub 160
IrqStub 161
IrqStub 162
IrqStub 163
IrqStub 164
IrqStub 165
IrqStub 166
IrqStub 167
IrqStub 168
IrqStub 169
IrqStub 170
IrqStub 171
IrqStub 172
IrqStub 173
IrqStub 174
IrqStub 175
IrqStub 176
IrqStub 177
IrqStub 178
IrqStub 179
IrqStub 180
IrqStub 181
IrqStub 182
IrqStub 183
IrqStub 184
IrqStub 185
IrqStub 186
IrqStub 187
IrqStub 188
IrqStub 189
IrqStub 190
IrqStub 191
IrqStub 192
IrqStub 193
IrqStub 194
IrqStub 195
IrqStub 196
IrqStub 197
IrqStub 198
IrqStub 199
IrqStub 200
IrqStub 201
IrqStub 202
IrqStub 203
IrqStub 204
IrqStub 205
IrqStub 206
IrqStub 207
IrqStub 208
IrqStub 209
IrqStub 210
IrqStub 211
IrqStub 212
IrqStub 213
IrqStub 214
IrqStub 215
IrqStub 216
IrqStub 217
IrqStub 218
IrqStub 219
IrqStub 220
IrqStub 221
IrqStub 222
IrqStub 223
IrqStub 224
IrqStub 225
IrqStub 226
IrqStub 227
IrqStub 228
IrqStub 229
IrqStub 230
IrqStub 231
IrqStub 232
IrqStub 233
IrqStub 234
IrqStub 235
IrqStub 236
IrqStub 237
IrqStub 238
IrqStub 239
IrqStub 240
IrqStub 241
IrqStub 242
IrqStub 243
IrqStub 244
IrqStub 245
IrqStub 246
IrqStub 247
IrqStub 248
IrqStub 249
IrqStub 250
IrqStub 251
IrqStub 252
IrqStub 253
IrqStub 254
IrqStub 255

section .data

g_pIdtIntTable:
    %assign i 0
    %rep 256
        dq IntStub%+i
        %assign i i+1
    %endrep