# 1_mem.s
.section .data
.align 2
my_data:
    .word 0x00120034         # word 0 at my_data (addresses aligned)
    .word 0xFFED2979         # word 1
    .byte 0x7F               # byte after words (for lb/lbu tests)
    .half 0x1234             # halfword (for lh/lhu test)

.section .text
.global main
main:
    # load address into register
    la      t0, my_data      # t0 -> &my_data

    lw      t1, 0(t0)        # t1 = word 0 (0x00120034)
    lw      t2, 4(t0)        # t2 = word 1 (0xFFED2979)
    lb      t3, 8(t0)        # t3 = sign-extended byte (0x7F)
    lbu     t4, 8(t0)        # t4 = zero-extended byte
    lh      t5, 10(t0)       # t5 = sign-extended half (0x1234)
    lhu     t6, 10(t0)       # t6 = zero-extended half

    # stores: modify memory
    li      t7, 0xAA
    sb      t7, 12(t0)       # store byte at my_data+12
    li      s0, 0xBBBB
    sh      s0, 13(t0)       # store half (note addresses ok for test)
    li      s1, 0xDEADBEEF
    sw      s1, 16(t0)       # store word at my_data+16

    li      a0, 1
    ret
