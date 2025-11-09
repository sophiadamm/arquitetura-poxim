# 1_imm_shift.s
.section .text
.global main
main:
    li      t0, 0x0000F00F
    li      t1, 0x00000004

    # I-type immediate arithmetic
    addi    s0, t0, 0x010    # s0 = t0 + 0x10
    andi    s1, t0, 0x0FF    # s1 = t0 & 0xFF
    ori     s2, t0, 0x00F0   # s2 = t0 | 0xF0
    xori    s3, t0, 0x0F0F   # s3 = t0 ^ 0x0F0F
    slti    s4, t0, 0x1000   # slti (signed compare)
    sltiu   s5, t0, 0x1000   # sltiu (unsigned compare)

    # shift immediate forms (slli, srli, srai)
    slli    s6, t0, 4
    srli    s7, t0, 4
    srai    s8, t0, 4

    li      a0, 1
    ret
