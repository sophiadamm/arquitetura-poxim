#
# Poxim-V software interruption example
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
# Software interruption handler
.global software_interruption_handler
software_interruption_handler:
    # Prologue
    addi sp, sp, -16
    sw ra, 0(sp)
    sw a0, 4(sp)
    sw t0, 8(sp)
    # Retrieving trap cause and status
    csrr a0, mcause
    csrr t0, mstatus
    # Completing software interruption
    li t0, 0x02000000
    sw zero, 0(t0)
    csrr t0, mip
    # Epilogue
    lw ra, 0(sp)
    lw a0, 4(sp)
    lw t0, 8(sp)
    addi sp, sp, 16
    # Returning from trap
    mret

# Trap configuration
trap_configuration:
    # Retrieving trap entry address (vectored)
    la t0, _trap_entry
    addi t0, t0, 1
    csrw mtvec, t0
    # Enabling software interruption at machine level (MSIE)
    csrr t0, mie
    ori t0, t0, 8
    csrw mie, t0
    # Enabling global interruption (MIE)
    csrr t0, mstatus
    ori t0, t0, 8
    csrw mstatus, t0
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
    # Claiming software interruption
    li t0, 0x02000000
    li t1, 1
    sw t1, 0(t0)
    # Epilogue
    lw ra, 0(sp)
    addi sp, sp, 16
    # Setting return value to zero (success)
    li a0, 0
    # Returning from call
    ret
