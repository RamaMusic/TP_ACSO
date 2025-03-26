.section .text
.global _start

_start:
    ADDS X0, X10, #1      // x0 = 0x1
    B skip                // salto incondicional

    ADDS X1, X10, #9      // estas no deben ejecutarse
    ADDS X2, X10, #8

skip:
    ADDS X3, X10, #3      // x3 = 0x3
    HLT #0
