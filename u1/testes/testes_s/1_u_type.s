# 1_u_type.s
.section .text
.global main
main:
    # LUI: load upper immed
    lui     t0, 0x12345        # t0 = 0x12345000

    # AUIPC: add upper imm to pc
    auipc   t1, 0x00002        # t1 = pc + 0x20000  (value depends on current pc)

    # use result to compute addresses and load/store safely
    addi    t2, t0, 0x10
    addi    t3, t1, 0x20

    li      a0, 1
    ret
