    .attribute arch, "rv64gc"
    .text
    .align 2
    .globl CallGeneric
    .type CallGeneric, @function
CallGeneric:
    addi sp, sp, -64
    sd s0, 0(sp)
    sd s1, 8(sp)
    sd s2, 16(sp)
    sd s3, 24(sp)
    sd s4, 32(sp)
    sd s5, 40(sp)
    sd s6, 48(sp)
    sd ra, 56(sp)

    mv s0, a0
    mv s1, a1
    mv s2, a2
    mv s3, a3
    mv s4, a4
    mv s5, a5
    mv s6, a6

    beqz s3, .Lload_floats
    ld a0, 0(s1)
    li t0, 1
    beq s3, t0, .Lload_floats
    ld a1, 8(s1)
    li t0, 2
    beq s3, t0, .Lload_floats
    ld a2, 16(s1)
    li t0, 3
    beq s3, t0, .Lload_floats
    ld a3, 24(s1)
    li t0, 4
    beq s3, t0, .Lload_floats
    ld a4, 32(s1)
    li t0, 5
    beq s3, t0, .Lload_floats
    ld a5, 40(s1)
    li t0, 6
    beq s3, t0, .Lload_floats
    ld a6, 48(s1)
    li t0, 7
    beq s3, t0, .Lload_floats
    ld a7, 56(s1)

.Lload_floats:
    beqz s4, .Ldo_call
    fld fa0, 0(s2)
    li t0, 1
    beq s4, t0, .Ldo_call
    fld fa1, 8(s2)
    li t0, 2
    beq s4, t0, .Ldo_call
    fld fa2, 16(s2)
    li t0, 3
    beq s4, t0, .Ldo_call
    fld fa3, 24(s2)
    li t0, 4
    beq s4, t0, .Ldo_call
    fld fa4, 32(s2)
    li t0, 5
    beq s4, t0, .Ldo_call
    fld fa5, 40(s2)
    li t0, 6
    beq s4, t0, .Ldo_call
    fld fa6, 48(s2)
    li t0, 7
    beq s4, t0, .Ldo_call
    fld fa7, 56(s2)

.Ldo_call:
    jalr s0

    li t0, 0
    beq s6, t0, .Lstore_int32
    li t0, 1
    beq s6, t0, .Lstore_int64
    li t0, 2
    beq s6, t0, .Lstore_double
    li t0, 3
    beq s6, t0, .Lstore_two_doubles
    j .Lepilogue

.Lstore_int32:
    sw a0, 0(s5)
    j .Lepilogue

.Lstore_int64:
    sd a0, 0(s5)
    j .Lepilogue

.Lstore_double:
    fsd fa0, 0(s5)
    j .Lepilogue

.Lstore_two_doubles:
    fsd fa0, 0(s5)
    fsd fa1, 8(s5)

.Lepilogue:
    ld s0, 0(sp)
    ld s1, 8(sp)
    ld s2, 16(sp)
    ld s3, 24(sp)
    ld s4, 32(sp)
    ld s5, 40(sp)
    ld s6, 48(sp)
    ld ra, 56(sp)
    addi sp, sp, 64
    ret
    .size CallGeneric, .-CallGeneric
