# -------------------------------
# Teste: Instruções tipo U e Loads
# -------------------------------

main:
    addi sp, sp, -16
    sw ra, 12(sp)

    # -------- Instruções U --------
    lui   a0, 0x12345          # a0 = 0x12345000
    auipc a1, 0x00001          # a1 = PC + 0x00001000

    # -------- Configuração base de dados --------
    # Vamos fingir que os dados começam em 0x80009000
    # e colocar manualmente os valores depois

    lui   a2, 0x80009          # a2 = 0x80009000 (endereço base)

    # -------- Loads --------
    lw    t0, 0(a2)            # deve carregar 0x00000BFF0A
    lh    t1, 2(a2)            # deve carregar 0x000B
    lhu   t2, 2(a2)            # idem, unsigned
    lb    t3, 1(a2)            # deve carregar 0xFFFFFFFF (-1)
    lbu   t4, 1(a2)            # deve carregar 0x000000FF

    lw ra, 12(sp)
    addi sp, sp, 16
    ret
