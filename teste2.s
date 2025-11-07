    .section .text
    .global main
main:
    addi sp, sp, -64
    sw ra, 60(sp)
    auipc gp, 1
    addi s0, zero, -4
    addi a0, zero, 5
    addi a1, zero, 3
    addi a2, zero, 1
    addi a3, zero, 1
    addi a4, zero, 2
    addi t0, zero, 16
    addi t1, zero, 2
    lui t2, 0x00010
    addi t2, t2, 0

    lb t3, 0(gp)
    lbu t4, 0(gp)
    lb s1, 1(gp)
    lh t5, 2(gp)
    lhu t6, 2(gp)
    lw s2, 4(gp)
    sb t0, 16(gp)
    sh t1, 18(gp)
    sw t2, 20(gp)

    slli t3, a0, 2
    srli t4, a0, 1
    srai t5, s0, 1
    sll t6, a0, t1
    srl s3, a0, t1
    sra s4, s0, t1

    slt s5, a1, a0
    sltu s6, s2, a0

    add s7, a0, a1
    sub t0, a0, a1
    and t1, a0, t5
    or t2, a0, a1
    xor t3, a0, a1

    mul t4, a0, a1
    mulh t5, a0, a1
    mulhsu t6, s0, a1
    mulhu s0, a0, a1

    div s1, a1, a0
    divu s2, a1, a0
    rem s3, a1, a0
    remu s4, a1, a0

    lui t5, 0x00012
    addi t6, t5, 256
    andi s5, t6, 240
    slti s6, a0, 8
    sltiu s7, a0, 7
    ori t0, a0, 32
    xori t1, a0, 3

    addi a5, zero, 0
    beq a2, a3, br_beq_taken
    addi a5, zero, 1
br_beq_taken:
    bne a2, a3, br_bne_taken
    addi a5, a5, 2
br_bne_taken:
    blt a2, a4, br_blt_taken
    addi a5, a5, 4
br_blt_taken:
    bge a4, a2, br_bge_taken
    addi a5, a5, 8
br_bge_taken:
    bltu a2, a4, br_bltu_taken
    addi a5, a5, 16
br_bltu_taken:
    bgeu a4, a2, br_bgeu_taken
    addi a5, a5, 32
br_bgeu_taken:

    jal ra, call1
    addi a5, a5, 64
call1:
    addi t0, t0, 1
    jalr zero, ra, 0

    addi a6, zero, 7
    addi a7, zero, 9

    add s1, a6, a7
    sub s2, a7, a6
    mul s3, a6, a7
    div s4, a7, a6
    rem s5, a7, a6

    slli s6, a7, 31
    srli s7, a7, 31
    srai t0, s0, 31

    sll t1, a7, a6
    srl t2, a7, a6
    sra t3, s0, a6

    mulh t4, a7, a6
    mulhsu t5, s0, a7
    mulhu t6, a7, a6

    divu s6, a7, a6
    remu s7, a7, a6

    addi t0, zero, 0x7F
    addi t1, zero, -1
    add t2, t0, t1
    sub t3, t1, t0
    and t4, t0, t1
    or t5, t0, t1
    xor t6, t0, t1

    auipc t0, 0x00002
    jal ra, call2
call2:
    addi t0, t0, 2
    jalr zero, ra, 0

    addi s0, s0, 1
    addi s1, s1, -1

    beq zero, zero, skip_traps
trap_block:
    ecall
    ebreak
skip_traps:

    lw t0, 20(gp)
    lh t1, 18(gp)
    lb t2, 16(gp)
    lbu t3, 16(gp)
    lhu t4, 18(gp)

    addi t5, zero, 123
    addi t6, zero, 456
    add s0, t5, t6
    mul s1, t5, t6
    div s2, t6, t5
    rem s3, t6, t5

    beq s3, zero, finish
    addi s3, s3, -1
    jal zero, finish
finish:
    lw ra, 60(sp)
    addi sp, sp, 64
    ret

    .section .data
    .word 0x0012
    .byte 0x34
    .hword 0x5678
    .word 0x89ABCDEF
    .space 64
