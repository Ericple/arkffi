    .intel_syntax noprefix
    .text
    .globl CallGeneric
    .type CallGeneric, @function
CallGeneric:
    push RBP
    push RBX
    push R12
    push R13
    push R14
    push R15
    push R9

    mov RBX, RDI
    mov R12, RSI
    mov R13, RDX
    mov R14D, ECX
    mov R15D, R8D

    test R14D, R14D
    jz .Lload_floats
    mov RDI, [R12]
    cmp R14D, 1
    je .Lload_floats
    mov RSI, [R12+8]
    cmp R14D, 2
    je .Lload_floats
    mov RDX, [R12+16]
    cmp R14D, 3
    je .Lload_floats
    mov RCX, [R12+24]
    cmp R14D, 4
    je .Lload_floats
    mov R8, [R12+32]
    cmp R14D, 5
    je .Lload_floats
    mov R9, [R12+40]

.Lload_floats:
    test R15D, R15D
    jz .Ldo_call
    movsd XMM0, [R13]
    cmp R15D, 1
    je .Ldo_call
    movsd XMM1, [R13+8]
    cmp R15D, 2
    je .Ldo_call
    movsd XMM2, [R13+16]
    cmp R15D, 3
    je .Ldo_call
    movsd XMM3, [R13+24]
    cmp R15D, 4
    je .Ldo_call
    movsd XMM4, [R13+32]
    cmp R15D, 5
    je .Ldo_call
    movsd XMM5, [R13+40]
    cmp R15D, 6
    je .Ldo_call
    movsd XMM6, [R13+48]
    cmp R15D, 7
    je .Ldo_call
    movsd XMM7, [R13+56]

.Ldo_call:
    call RBX

    mov RCX, [RSP]
    mov EDX, [RSP+64]

    cmp EDX, 0
    je .Lstore_int32
    cmp EDX, 1
    je .Lstore_int64
    cmp EDX, 2
    je .Lstore_double
    cmp EDX, 3
    je .Lstore_two_doubles
    jmp .Lepilogue

.Lstore_int32:
    mov [RCX], EAX
    jmp .Lepilogue

.Lstore_int64:
    mov [RCX], RAX
    jmp .Lepilogue

.Lstore_double:
    movsd [RCX], XMM0
    jmp .Lepilogue

.Lstore_two_doubles:
    movsd [RCX], XMM0
    movsd [RCX+8], XMM1

.Lepilogue:
    pop R9
    pop R15
    pop R14
    pop R13
    pop R12
    pop RBX
    pop RBP
    ret
    .size CallGeneric, .-CallGeneric
