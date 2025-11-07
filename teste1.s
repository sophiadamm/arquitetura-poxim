# test_all_instructions.S
# .text at 0x80000000, .data at 0x80001000
# Usa só registradores: zero, ra, sp, gp, tp, t0-t6, s0-s11, a0-a7
# Evita exceções — divisões têm divisor != 0; ebreak/ecall não são executados.

    .section .text
    .org 0x80000000
    .global main

# -------------------------------------------------------------------
# Main entry (prologue)
# -------------------------------------------------------------------
main:
    # save ra and make small stack frame
    addi sp, sp, -32
    sw ra, 28(sp)

# Setup: point gp to .data at 0x80001000 using auipc (pc = 0x80000000)
# 0x80000000:auipc gp,0x00001    gp = 0x80000000 + 0x00001_000 = 0x80001000
    auipc gp, 0x00001

# Load some small immediates using addi (no li pseudo-op)
# 0x80000004:lui s0,0x00000      s0 = 0x00000000_000 => we'll set s0 = -4 via addi next
    addi s0, zero, -4          # s0 = 0xFFFFFFFC

# Setup registers used for tests
# a0 = 5, a1 = 3, t0 = 0x10, t1 = 2, t2 = 2 (shift amount)
# 0x80000008:addi a0,zero,0x5    a0 = 0x00000005
    addi a0, zero, 5
# 0x8000000c:addi a1,zero,0x3    a1 = 0x00000003
    addi a1, zero, 3
# 0x80000010:addi t0,zero,0x10   t0 = 0x00000010
    addi t0, zero, 16
# 0x80000014:addi t1,zero,0x2    t1 = 0x00000002
    addi t1, zero, 2
# 0x80000018:addi t2,zero,0x2    t2 = 0x00000002 (used as rs2 shift amount)
    addi t2, zero, 2

# -------------------------
# LOADS (from .data via gp)
# -------------------------
# data base: gp = 0x80001000
# At .data we placed:
# 0x80001000: .byte 0x12
# 0x80001002: .hword 0x1234
# 0x80001004: .word 0x89ABCDEF

# 0x8000001c:lb   t3,0x000(gp)     t3 = mem[0x80001000] = 0x12 -> sign-extended 0x00000012
    lb t3, 0(gp)

# 0x80000020:lbu  t4,0x000(gp)     t4 = mem[0x80001000] = 0x12 -> zero-extended 0x00000012
    lbu t4, 0(gp)

# 0x80000024:lh   t5,0x002(gp)     t5 = mem[0x80001002] = 0x1234 -> sign-extended 0x00001234
    lh t5, 2(gp)

# 0x80000028:lhu  t6,0x002(gp)     t6 = mem[0x80001002] = 0x1234 -> zero-extended 0x00001234
    lhu t6, 2(gp)

# 0x8000002c:lw   s1,0x004(gp)     s1 = mem[0x80001004] = 0x89ABCDEF
    lw s1, 4(gp)

# -------------------------
# STORES (write into .data+0x10..0x1C reserved)
# -------------------------
# Prepare store values
# 0x80000030:addi t3,zero,0xAA     t3 = 0x000000AA (will store byte)
    addi t3, zero, 0xAA
# 0x80000034:addi t4,zero,0xBBBB   t4 = 0x0000BBBB (will store half)
    addi t4, zero, 0xBBBB
# 0x80000038:addi t5,zero,0xDEADBEEF (cannot fit 12-bit imm) -> build via lui/addi
    lui t5, 0xDEADB  # t5 = 0xDEADB000
    addi t5, t5, 0xEEF  # t5 = 0xDEADBEEF

# 0x8000003c:sb   t3,0x010(gp)     mem[0x80001010] = 0xAA
    sb t3, 0x10(gp)

# 0x80000040:sh   t4,0x012(gp)     mem[0x80001012] = 0xBBBB
    sh t4, 0x12(gp)

# 0x80000044:sw   t5,0x014(gp)     mem[0x80001014] = 0xDEADBEEF
    sw t5, 0x14(gp)

# -------------------------
# Shifts: immediate and register-shift
# -------------------------
# slli, srli, srai (immediate u5)
# 0x80000048:slli   t6,a0,2       t6 = 0x5 << 2 = 0x14
    slli t6, a0, 2

# 0x8000004c:srli   t7,a0,1       t7 = 0x5 >> 1 = 0x2
    srli t7, a0, 1

# 0x80000050:srai   t0,s0,1       s0=-4 (0xFFFFFFFC) >>> arith -> -2 = 0xFFFFFFFE
    srai t0, s0, 1

# sll/srl/sra (shift amount in rs2)
# 0x80000054:sll    t1,a0,t2       t1 = 0x5 << (t2&0x1F=2) = 0x14
    sll t1, a0, t2

# 0x80000058:srl    t2,a0,t1       t2 = 0x5 >> (t1&0x1F) ; t1 currently 0x14 -> &0x1F=20 -> 5>>20 = 0
    srl t2, a0, t1

# 0x8000005c:sra    t3,s0,t1       t3 = s0 >> (t1&0x1F) ; s0=-4 >>20 = -1 (arith shift)
    sra t3, s0, t1

# -------------------------
# Comparisons and arithmetic/logical
# -------------------------
# slt, sltu
# 0x80000060:slt   t4,a1,a0       t4 = (3 < 5) = 1
    slt t4, a1, a0

# 0x80000064:sltu  t5,s1,a0       t5 = (0x89ABCDEF < 5 unsigned) = 0 (false)
    sltu t5, s1, a0

# add, sub, and, or, xor
# 0x80000068:add   t6,a0,a1       t6 = 5 + 3 = 8 (0x8)
    add t6, a0, a1

# 0x8000006c:sub   t7,a0,a1       t7 = 5 - 3 = 2 (0x2)
    sub t7, a0, a1

# 0x80000070:and   t0,a0,t0       t0 = 5 & 0xFFFFFFFE = 0x4
    and t0, a0, t0

# 0x80000074:or    t1,a0,a1       t1 = 5 | 3 = 7 (0x7)
    or t1, a0, a1

# 0x80000078:xor   t2,a0,a1       t2 = 5 ^ 3 = 6 (0x6)
    xor t2, a0, a1

# -------------------------
# Multiply family
# -------------------------
# 0x8000007c:mul    t3,a0,a1       t3 = 5 * 3 = 15 (0xF)
    mul t3, a0, a1

# mulh, mulhsu, mulhu — results are high 32 bits of 64-bit product
# choose operands small so mulh = 0 (valid test)
# 0x80000080:mulh   t4,a0,a1      t4 = high32(5*3) = 0
    mulh t4, a0, a1

# 0x80000084:mulhsu t5,s0,a1      t5 = high32(signed(-4)*unsigned(3)) ; (-4)*3 = -12 -> high32 = 0xFFFFFFFF (implementation-defined sign extension) -> valid test
    mulhsu t5, s0, a1

# 0x80000088:mulhu  t6,a0,a1      t6 = high32(unsigned(5)*unsigned(3)) = 0
    mulhu t6, a0, a1

# -------------------------
# Div/Rem family — ensure divisors != 0
# -------------------------
# 0x8000008c:div    t7,a1,a0       t7 = 3 / 5 (signed) = 0
    div t7, a1, a0

# 0x80000090:divu   t0,a1,a0       t0 = 3 / 5 (unsigned) = 0
    divu t0, a1, a0

# 0x80000094:rem    t1,a1,a0       t1 = 3 % 5 (signed) = 3
    rem t1, a1, a0

# 0x80000098:remu   t2,a1,a0       t2 = 3 % 5 (unsigned) = 3
    remu t2, a1, a0

# -------------------------
# AUIPC / LUI / ADDI / Logical immediates
# -------------------------
# 0x8000009c:lui    t3,0x00012     t3 = 0x00012_000 = 0x00012000
    lui t3, 0x00012

# 0x800000a0:addi   t4,t3,0x100    t4 = 0x00012000 + 0x100 = 0x00012100
    addi t4, t3, 0x100

# 0x800000a4:andi   t5,t4,0x0F0    t5 = 0x00012100 & 0xF0 = 0x100 & 0xF0 = 0x00
    andi t5, t4, 0x0F0

# 0x800000a8:slti   t6,a0,0x8      t6 = (5 < 8) ? 1 : 0 = 1
    slti t6, a0, 8

# 0x800000ac:sltiu  t7,a0,0xFFFFFFFF (use small positive imm 0x7) -> sltiu t7,a0,7
    sltiu t7, a0, 7

# 0x800000b0:ori    t0,a0,0x20     t0 = 5 | 0x20 = 0x25
    ori t0, a0, 0x20

# 0x800000b4:xori   t1,a0,0x3      t1 = 5 ^ 3 = 6 (0x6)
    xori t1, a0, 3

# -------------------------
# Branches (take and not-take)
# -------------------------
# We'll use labels and show whether branches are taken in comments.

# Prepare registers for branch tests
# a2 = 1, a3 = 1, a4 = 2
    addi a2, zero, 1
    addi a3, zero, 1
    addi a4, zero, 2

# 0x800000b8:beq a2,a3,BEQ_TAKEN     (1 == 1) = 1 -> pc -> BEQ_TAKEN
    beq a2, a3, beq_taken
# 0x800000bc: (not executed if branch taken)
    addi t2, zero, 0xDEAD  # would be skipped when branch taken

beq_taken:
# comment: beq taken -> jumped to label beq_taken
# continue

# 0x800000c0:bne a2,a3,BNE_NOTTAKEN  (1 != 1) = 0 -> not taken -> fallthrough
    bne a2, a3, bne_taken
# fallthrough

# 0x800000c4:blt a2,a4,BLT_TAKEN     (1 < 2) = 1 -> taken
    blt a2, a4, blt_taken
# unreachable if taken

blt_taken:
# 0x800000c8:bge a4,a2,BGE_TAKEN     (2 >= 1) = 1 -> taken
    bge a4, a2, bge_taken

bge_taken:
# 0x800000cc:bltu a2,a4,BLTU_TAKEN   (1u < 2u) = 1 -> taken
    bltu a2, a4, bltu_taken

bltu_taken:
# 0x800000d0:bgeu a4,a2,BGEU_TAKEN   (2u >= 1u) = 1 -> taken
    bgeu a4, a2, bgeu_taken

bgeu_taken:
# -------------------------
# Jumps: jal, jalr
# -------------------------
# 0x800000d4:jal ra,call_site    -> sets ra to next instr addr and jumps
    jal ra, call_site

# fallthrough if not executed? jal jumped, so next instruction below won't run until return.

# call_site:
call_site:
# 0x800000d8: (in called site) --- simple operation
    addi t0, t0, 1
# 0x800000dc:jalr zero,ra,0     -> return by jumping to ra (jalr rd=zero doesn't write link)
    jalr zero, ra, 0

# -------------------------
# Put ebreak and ecall in code but after a branch to skip them (so they are not executed)
# -------------------------
    j after_traps        # jump over the trap instructions

# These exist in the binary (so "included") but are not executed in normal run:
trap_area:
# 0x800000e8:ecall
    ecall
# 0x800000ec:ebreak
    ebreak

after_traps:
# epilogue
    lw ra, 28(sp)
    addi sp, sp, 32
    ret

# -------------------------------------------------------------------
# Data section (at 0x80001000)
# -------------------------------------------------------------------
    .section .data
    .org 0x80001000
data_base:
    .byte 0x12           # 0x80001000
    .byte 0x00
    .hword 0x1234        # 0x80001002
    .word 0x89ABCDEF     # 0x80001004
    .space 0x20          # space for stores; e.g. sb/sh/sw targets at offsets 0x10/0x12/0x14
