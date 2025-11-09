# 1_rtype.s
.section .text
.global main
main:
    # prologue (opcional, poxim-v.s cuida do _start)
    # Teste de add/sub/and/or/xor/slt/sltu/sll/srl/sra
    li      t0, 0x00000005      # t0 = 5
    li      t1, 0xFFFFFFFE      # t1 = -2 (0xFFFFFFFE)
    li      t2, 0x00000003      # t2 = 3

    add     t3, t0, t2          # t3 = 5 + 3 = 8
    sub     t4, t0, t1          # t4 = 5 - (-2) = 7
    and     t5, t0, t1          # t5 = 5 & -2
    or      t6, t0, t1          # t6 = 5 | -2
    xor     t7, t0, t1          # t7 = 5 ^ -2

    sll     s0, t0, t2          # s0 = 5 << 3 = 40
    srl     s1, t1, t2          # s1 = (unsigned) -2 >> 3
    sra     s2, t1, t2          # s2 = (signed) -2 >> 3 (arith shift)

    slt     s3, t1, t0          # s3 = (-2 < 5) => 1
    sltu    s4, t1, t0          # s4 = (uint32(-2) < 5) => 0

    # return success in a0
    li      a0, 1
    ret
