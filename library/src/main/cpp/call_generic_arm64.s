    .arch armv8-a
    .text
    .align 2
    .global CallGeneric
    .type CallGeneric, @function
CallGeneric:
    stp x29, x30, [sp, #-80]!
    stp x19, x20, [sp, #16]
    stp x21, x22, [sp, #32]
    stp x23, x24, [sp, #48]
    str x25, [sp, #64]
    mov x29, sp

    mov x19, x0
    mov x20, x1
    mov x21, x2
    mov w22, w3
    mov w23, w4
    mov x24, x5
    mov w25, w6

    cbz w22, load_floats
    ldr x0, [x20]
    cmp w22, #1
    b.eq load_floats
    ldr x1, [x20, #8]
    cmp w22, #2
    b.eq load_floats
    ldr x2, [x20, #16]
    cmp w22, #3
    b.eq load_floats
    ldr x3, [x20, #24]
    cmp w22, #4
    b.eq load_floats
    ldr x4, [x20, #32]
    cmp w22, #5
    b.eq load_floats
    ldr x5, [x20, #40]
    cmp w22, #6
    b.eq load_floats
    ldr x6, [x20, #48]
    cmp w22, #7
    b.eq load_floats
    ldr x7, [x20, #56]

load_floats:
    cbz w23, do_call
    ldr d0, [x21]
    cmp w23, #1
    b.eq do_call
    ldr d1, [x21, #8]
    cmp w23, #2
    b.eq do_call
    ldr d2, [x21, #16]
    cmp w23, #3
    b.eq do_call
    ldr d3, [x21, #24]
    cmp w23, #4
    b.eq do_call
    ldr d4, [x21, #32]
    cmp w23, #5
    b.eq do_call
    ldr d5, [x21, #40]
    cmp w23, #6
    b.eq do_call
    ldr d6, [x21, #48]
    cmp w23, #7
    b.eq do_call
    ldr d7, [x21, #56]

do_call:
    blr x19

    cmp w25, #0
    b.eq store_int32
    cmp w25, #1
    b.eq store_int64
    cmp w25, #2
    b.eq store_double
    cmp w25, #3
    b.eq store_two_doubles
    b epilogue

store_int32:
    str w0, [x24]
    b epilogue

store_int64:
    str x0, [x24]
    b epilogue

store_double:
    str d0, [x24]
    b epilogue

store_two_doubles:
    stp d0, d1, [x24]

epilogue:
    ldp x19, x20, [sp, #16]
    ldp x21, x22, [sp, #32]
    ldp x23, x24, [sp, #48]
    ldr x25, [sp, #64]
    ldp x29, x30, [sp], #80
    ret
    .size CallGeneric, .-CallGeneric
