#Infos gerais:
#a0 -> base da uart (RHR/THR) 
#a1 -> endereço do vetor
#t0 -> acumulador
#s1 -> contar o loop

#Code section
.section .text

# Main function
.global main
main:
    addi sp, sp, -16
    sw ra, 0(sp)

    li a0, 0x10000000      # base da uart (valor do inteiro)  
    la a1, vetor           # la -> load addres

    jal ra, ler_inteiro

    mv s1, a0           # s1 = N

    lw ra, 0(sp)
    addi sp, sp, 16
    li a0, 0
    ret


# Polling - com esperas
ler_inteiro:
    li t0, 0            # acumulador
    li t4, 1            # sinal (1 = positivo, -1 = negativo)

loop_leitura: 
    # Verificar se tem o dado - LSR (deslocamento de 5 da base)
    lb t1, 5(a0)        
    andi t1, t1, 1      # isola o bit 0 (Data Ready)
    beq t1, zero, loop_leitura  # Se for 0, não tem dado. Volta e espera.

    # Ler o dado de RHR
    lb t2, 0(a0)       

    li t3, 32           # ASCII do Espaço
    beq t2, t3, fim_leitura

    li t3, 45           # AISCII de -
    beq t2, t3, trata_negativo

    li t3, 10           # ASCII do "\n" e valor para multiplicar dps
    beq t2, t3, fim_leitura

    addi t2, t2, -48    # aiscii -> inteiro
    
    mul t0, t0, t3   
    add t0, t0, t2 

    j loop_leitura

trata_negativo:
    li t4, -1    
    j loop_leitura

fim_leitura:
    mul a0, t0, t4
    ret                 # Volta para a main

# Data section
.section .data
vetor:
    .zero 4000      