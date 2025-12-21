    .section .text
    .global main
main:
    addi sp, sp, -16
    sw ra, 12(sp)

    # Inicializa registradores
    addi a0, zero, 5
    addi a1, zero, 5
    addi a2, zero, 3
    addi a3, zero, 8
    addi t0, zero, 0     # contador

    # BEQ (deve saltar)
    beq a0, a1, igual
    addi t0, t0, 1
igual:
    addi t0, t0, 10

    # BNE (não deve saltar)
    bne a0, a1, dif
    addi t0, t0, 2
dif:
    addi t0, t0, 20

    # BLT (deve saltar: 3 < 5)
    blt a2, a0, menor
    addi t0, t0, 3
menor:
    addi t0, t0, 30

    # BGE (não deve saltar: 3 >= 5 é falso)
    bge a2, a0, maior_igual
    addi t0, t0, 4
maior_igual:
    addi t0, t0, 40

    # BLTU (deve saltar: 3 < 8 unsigned)
    bltu a2, a3, menor_u
    addi t0, t0, 5
menor_u:
    addi t0, t0, 50

    # BGEU (deve saltar: 8 >= 5 unsigned)
    bgeu a3, a0, maior_u
    addi t0, t0, 6
maior_u:
    addi t0, t0, 60

    lw ra, 12(sp)
    addi sp, sp, 16
    ret
