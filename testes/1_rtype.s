# 1_rtype.s
.section .text
.global main
main:
    # prologue (opcional)
    # Teste de add/sub/and/or/xor/slt/sltu/sll/srl/sra
    li      t0, 5            # t0 = 5
    li      t1, -2           # t1 = -2 (0xFFFFFFFE)
    li      t2, 3            # t2 = 3

    add     t3, t0, t2       # t3 = 5 + 3 = 8
    sub     t4, t0, t1       # t4 = 5 - (-2) = 7
    and     t5, t0, t1       # t5 = 5 & -2
    or      t6, t0, t1       # t6 = 5 | -2
    xor     s0, t0, t1       # xor em s0 (usei s0 no lugar de t7)

    sll     s1, t0, t2       # s1 = 5 << 3 = 40
    srl     s2, t1, t2       # s2 = (unsigned) -2 >> 3
    sra     s3, t1, t2       # s3 = (signed) -2 >> 3 (arith shift)

    slt     s4, t1, t0       # s4 = (-2 < 5) => 1
    sltu    s5, t1, t0       # s5 = (uint32(-2) < 5) => 0

    # return success
    li      a0, 1
    ret
