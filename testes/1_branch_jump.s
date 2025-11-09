# 1_branch_jump.s
.section .text
.global main
main:
    li      t0, 10
    li      t1, 10
    li      t2, 5
    li      t3, 0x80000000

    beq     t0, t1, L_equal      # equal -> jump
    # not equal path:
    addi    s0, zero, 0
    j       L_after_eq

L_equal:
    addi    s0, zero, 1

L_after_eq:
    bne     t0, t2, L_noteq
    addi    s1, zero, 0
    j       L_after_ne

L_noteq:
    addi    s1, zero, 1

L_after_ne:
    blt     t2, t0, L_less
    addi    s2, zero, 0
    j       L_after_lt

L_less:
    addi    s2, zero, 1

L_after_lt:
    bltu    t2, t3, L_bltu     # unsigned compare (t2 < t3 true)
    addi    s3, zero, 0
    j       L_after_bltu

L_bltu:
    addi    s3, zero, 1

L_after_bltu:
    # test jal (jump-and-link)
    jal     ra, L_jal_target   # saves return addr in ra
    # will continue at next instruction after link when target does jalr/ret
    addi    s4, zero, 0        # this will be skipped because jal jumped
L_jal_target:
    addi    s4, zero, 1
    # return from the linked call via jalr
    jalr    zero, ra, 0        # jump back (pc = ra & ~1)

    # test jalr direct: make a small function-like jump
    la      t0, L_jalr_target2
    addi    t1, zero, 0
    jalr    ra, 0(t0)          # ra = pc+4 ; pc = t0 & ~1
    addi    s5, zero, 0        # skipped
L_jalr_target2:
    addi    s5, zero, 1
    jalr    zero, ra, 0        # return

    li      a0, 1
    ret
