#
# Poxim-V exception handling example
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
# Exception handler
.global exception_handler
exception_handler:
    # Prologue
    addi sp, sp, -16
    sw ra, 0(sp)
    sw a0, 4(sp)
    sw t0, 8(sp)
    sw t1, 12(sp)
    # Retrieving mcause, mepc and mstatus
    csrr a0, mcause
    csrr t0, mepc
    csrr t1, mstatus
    # Checking for instruction access fault (1)
    li t1, 1
    bne a0, t1, increment_mepc
    la t1, exceptions
    add t0, t0, t1
    # Incrementing mepc by 4
    increment_mepc:
        addi t0, t0, 4
        csrw mepc, t0
    # Epilogue
    lw ra, 0(sp)
    lw a0, 4(sp)
    lw t0, 8(sp)
    lw t1, 12(sp)
    addi sp, sp, 16
    # Returning from trap
    mret

# Trap configuration
trap_configuration:
    # Retrieving trap entry address (direct)
    la t0, _trap_entry
    csrw mtvec, t0
    # Returning from call
    ret

# Main function
.global main
main:
    # Prologue
    addi sp, sp, -16
    sw ra, 0(sp)
    # Trap handling configuration
    call trap_configuration
    # Exceptions
    exceptions:
        # Instruction access fault (1)
        jr zero
        # Illegal instruction (2)
        .word 0xf0c0f0fe
        # Load access fault (5)
        lw zero, 1(zero)
        # Store access fault (7)
        sw zero, 3(zero)
        # Environment call
        ecall
    # Retrieving mstatus
    csrr t0, mstatus
    # Epilogue
    lw ra, 0(sp)
    addi sp, sp, 16
    # Setting return value to zero (success)
    li a0, 0
    # Returning from call
    ret
