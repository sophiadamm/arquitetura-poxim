#
# Poxim-V UART interruption example
# 
# (C) Copyright 2024 Bruno Otavio Piedade Prado
#
# This file is part of Poxim-V.
#
# Poxim-V is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Poxim-V is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Poxim-V.  If not, see <https://www.gnu.org/licenses/>.
#

# Code section
.section .text
# External interruption handler
.global external_interruption_handler
external_interruption_handler:
    # Prologue
    addi sp, sp, -16
    sw ra, 0(sp)
    sw a0, 4(sp)
    sw t0, 8(sp)
    # Retrieving trap cause
    csrr a0, mcause
    # Claiming pending interruption
    lw t0, 4(a4)
    # Incrementing string index
    addi a5, a5, 1
    # Completing pending interruption
    sw t0, 4(a4)
    # Epilogue
    lw ra, 0(sp)
    lw a0, 4(sp)
    lw t0, 8(sp)
    addi sp, sp, 16
    # Returning from trap
    mret

# PLIC configuration
plic_configuration:
    # Setting PLIC UART priority to 1 (source 10)
    li t0, 1
    sw t0, 40(a1)
    # Checking PLIC UART pending interruption (bit 10)
    lw t0, 0(a2)
    # Enable PLIC UART interruption (bit 10)
    li t0, 0x400
    sw t0, 0(a3)
    # Checking PLIC priority threshold for context 0
    lw t0, 0(a4)
    # Returning from call
    ret

# Send data
send_data:
    # Send data loop
    send_data_loop:
        # Reading byte from string
        lb t0, 0(a5)
        # Checking for null character
        beq t0, zero, send_data_end
        # Sending byte to UART
        sb t0, 0(a0)
        # Branching to loop
        j send_data_loop
    # Send data end
    send_data_end:
        # Returning from call
        ret

# Trap configuration
trap_configuration:
    # Retrieving trap entry address (vectored)
    la t0, _trap_entry
    addi t0, t0, 1
    csrw mtvec, t0
    # Enabling external interruption at machine level (MEIE)
    csrr t0, mie
    li t1, 0x800
    or t0, t0, t1
    csrw mie, t0
    # Enabling global interruption (MIE)
    csrr t0, mstatus
    ori t0, t0, 0x08
    csrw mstatus, t0
    # Returning from call
    ret

# UART configuration
uart_configuration:
    # Checking UART interruption status
    lb t0, 2(a0)
    # Enabling UART transmission interruption
    li t0, 2
    sb t0, 1(a0)
    # Returning from call
    ret

# Main function
.global main
main:
    # Prologue
    addi sp, sp, -16
    sw ra, 0(sp)
    # Setting UART address
    li a0, 0x10000000
    # Setting PLIC priority, pending, enable and threshold/claim/complete addresses
    li a1, 0x0c000000
    li a2, 0x0c001000
    li a3, 0x0c002000
    li a4, 0x0c200000
    # Retrieving numbers string
    la a5, numbers
    # UART configuration
    call uart_configuration
    # PLIC configuration
    call plic_configuration
    # Trap handling configuration
    call trap_configuration
    # Sending data via UART
    call send_data
    # Epilogue
    lw ra, 0(sp)
    addi sp, sp, 16
    # Setting return value to zero (success)
    li a0, 0
    # Returning from call
    ret

# Read-only data section
.section .rodata
# 123 string
numbers:
    .string "123\n"
