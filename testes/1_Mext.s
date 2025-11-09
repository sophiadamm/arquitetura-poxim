# 1_Mext.s
.section .text
.global main
main:
    # prepare operands
    li      a0, 0x0012d687      # a0 positive small
    li      a1, 0xffed2979      # a1 large unsigned (same bits as example)
    li      a2, 0x00000002      # a2 = 2

    mul     t0, a0, a2          # low 32 bits of (a0*a2)
    mulh    t1, a0, a1          # high 32 bits signed* signed
    mulhsu  t2, a0, a1          # high 32 bits signed * unsigned
    mulhu   t3, a1, a2          # high 32 bits unsigned*unsigned

    div     s0, a0, a2          # signed division
    divu    s1, a1, a2          # unsigned division
    rem     s2, a0, a2          # signed remainder
    remu    s3, a1, a2          # unsigned remainder

    # Check division by zero behavior is defined: use nonzero divisors here.

    li      a0, 1
    ret
