

# Code section
.section .text
# printf function
printf:
    # printf loop
    printf_loop:
        # Reading string character
        lb t0, 0(a1)
        # Checking for null character
        beq t0, zero, printf_end
        # Outputting character
        sb t0, 0(a0)
        # Incrementing index
        addi a1, a1, 1
        # Branching to loop
        j printf_loop
    # printf end
    printf_end:
        # Returning from call
        ret

# scanf function
scanf:
    # Scan loop
    scanf_loop:
        # Reading string character
        lb t0, 0(a0)
        # Storing character
        sb t0, 0(a1)
        # Incrementing index
        addi a1, a1, 1
        # Retrieving Data Ready fom Line Status Register (LSR)
        lb t0, 5(a0)
        andi t0, t0, 1
        # Branching to loop
        bne t0, zero, scanf_loop
    # Appending null character in string
    sb zero, 0(a1)
    # Returning from call
    ret

# Main function
.global main
main:
    # Prologue
    addi sp, sp, -16
    sw ra, 0(sp)
    # Setting UART0 address
    li a0, 0x10000000
    # Setting string address
    la a1, input
    # Calling scanf function
    call scanf
    # Setting string address
    la a1, input
    # Calling printf function
    call printf
    # Setting string address
    la a1, hello_world
    # Calling printf function
    call printf
    # Epilogue
    lw ra, 0(sp)
    addi sp, sp, 16
    # Setting return value to zero (success)
    li a0, 0
    # Returning from call
    ret

# Read-only data section
.section .rodata
# Hello world string
hello_world:
    .string "Hello world!\n"

# Data section
.section .data
# Input string (99 characters allocated + '\0')
input:
    .zero 100