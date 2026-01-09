#Infos gerais:
#s0 -> base da uart (RHR/THR) 
#a0 -> valor de retorno
#t0 -> acumulador
#s1 -> contar o loop
#s2 -> endereco do vetor
#s3 -> guardar o N

#Code section
.section .text

# Main function
.global main
main:
    addi sp, sp, -32
    sw ra, 0(sp)
    sw s0, 4(sp)            
    sw s1, 8(sp)            
    sw s2, 12(sp)           
    sw s3, 16(sp)          

    li s0, 0x10000000     # base da uart (valor do inteiro - retorno)  
    jal ra, ler_inteiro

    mv s1, a0           # s1 = N
    mv s3, a0           # s3 = N
    la s2, vetor        # s2 = Ponteiro atual do vetor

    beq s1, zero, fim_programa     # se N == 0

loop_preencher_vetor:

    jal ra, ler_inteiro
    sw a0, 0(s2)   

    # Avança o ponteiro do vetor para a próxima posição
    addi s2, s2, 4      
    
    # Decrementa o contador N
    addi s1, s1, -1
    
    bne s1, zero, loop_preencher_vetor

    mv a0, s3           
    la a1, vetor       
    jal ra, bubble_sort

    mv s1, s3           
    la s2, vetor        

loop_imprimir_vetor:
    lw a0, 0(s2)
    jal ra, escrever_inteiro
    
    addi s2, s2, 4      # s2++
    addi s1, s1, -1     # s1--
    
    bne s1, zero, loop_imprimir_vetor

fim_programa:
    # --- EPILOGO ---
    lw ra, 0(sp)
    lw s0, 4(sp)            
    lw s1, 8(sp)
    lw s2, 12(sp)
    lw s3, 16(sp)
    addi sp, sp, 32         
    li a0, 0
    ret


# -----------------------------------------------------------
# Ler um inteiro
# -----------------------------------------------------------
ler_inteiro:
    li t0, 0            # acumulador
    li t4, 1            # sinal (1 = positivo, -1 = negativo)

loop_leitura: 
    # Verificar se tem o dado - LSR (deslocamento de 5 da base)
    lb t1, 5(s0)       
    andi t1, t1, 1      # isola o bit 0 (Data Ready)
    beq t1, zero, loop_leitura  # Se for 0, não tem dado. Volta e espera.

    # Ler o dado de RHR
    lb t2, 0(s0)       

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
    ret     

# -----------------------------------------------------------
# Bubble Sort
# -----------------------------------------------------------
bubble_sort:
    li t0, 0                # i = 0

loop_externo:
    bge t0, a0, fim_sort    # Se i >= N
    
    li t1, 0                # j = 0
    addi t2, a0, -1         # Limite = N - 1

loop_interno:
    bge t1, t2, incremento_i # Se j >= N-1

    # endereço = base + (j * 4)
    slli t3, t1, 2          
    add t3, a1, t3  

    # --- LER VALORES ---
    lw t4, 0(t3)            # t4 = vetor[j]
    lw t5, 4(t3)            # t5 = vetor[j+1]

    ble t4, t5, incremento_j

    sw t5, 0(t3)          
    sw t4, 4(t3)            

incremento_j:
    addi t1, t1, 1          # j++
    j loop_interno

incremento_i:
    addi t0, t0, 1          # i++
    j loop_externo

fim_sort:
    ret 

# -----------------------------------------------------------
# Transmissão para UART
# -----------------------------------------------------------
escrever_inteiro:
    li t5, 32           # ASCII Espaço
    bne a0, zero, verifica_sinal

    #se tá aqui é porque a0 guar o valor 0
    li t0, 48           # ASCII '0'
    sb t0, 0(s0)        # Escreve na UART
    sb t5, 0(s0)
    ret

verifica_sinal:
    bge a0, zero, inic_empilha  # a0 >= 0
    
    # Se for negativo:
    li t0, 45           # ASCII '-'
    sb t0, 0(s0)        # Imprime o sinal
    sub a0, zero, a0    # Inverte o sinal pra conta

inic_empilha:
    li t3, 0            # Contador de dígitos empilhados
    li t4, 10           # Divisor

loop_empilha:
    beq a0, zero, loop_desempilha # Se o número acabou (ficou 0)
    
    rem t1, a0, t4      # t1 = a0 % 10 
    div a0, a0, t4      # a0 = a0 / 10
    
    addi t1, t1, 48     # inteiro -> aiscii
    
    # stack temporária
    addi sp, sp, -4
    sw t1, 0(sp)
    
    addi t3, t3, 1
    j loop_empilha

loop_desempilha:
    beq t3, zero, fim_escrita   # t3 == 0
    
    lw t0, 0(sp)
    addi sp, sp, 4  
    
    sb t0, 0(s0)        # Manda pra UART
    
    addi t3, t3, -1     # t3--
    j loop_desempilha

fim_escrita:
    sb t5, 0(s0)
    ret

# Data section
.section .data
vetor:
    .zero 4000      
