    .section .text
    .global main
main:
    addi sp, sp, -16
    sw ra, 12(sp)

    # Instruções tipo I e R
    addi a0, zero, 5
    addi a1, zero, 3
    add a2, a0, a1       # a2 = 8
    sub a3, a0, a1       # a3 = 2

    # Instruções lógicas
    and a4, a0, a1
    or  a5, a0, a1
    xor a6, a0, a1

    # Shift
    slli a7, a0, 1       # a7 = 10
    srli t0, a7, 1       # t0 = 5

    # Comparações
    slt  t1, a1, a0      # 3 < 5 → t1 = 1
    sltu t2, a0, a1      # unsigned 5 < 3 → t2 = 0

    # Multiplicação e divisão
    mul  t3, a0, a1      # 5*3 = 15
    div  t4, a0, a1      # 5/3 = 1
    rem  t5, a0, a1      # 5%3 = 2

    # Branch
    beq  a2, a2, label_ok
    addi a0, zero, 0
label_ok:
    addi a0, a0, 1

    # Load / Store
    la   t6, dado
    sw   a2, 0(t6)
    lw   t1, 0(t6)

    # Jump
    jal  ra, pula
    addi a0, a0, 2
pula:
    addi a0, a0, 3

    lw ra, 12(sp)
    addi sp, sp, 16
    ret

    .section .data
dado: .word 0
