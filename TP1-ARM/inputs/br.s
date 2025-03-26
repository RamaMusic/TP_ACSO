.section .text
.global _start

_start:
    MOV X0, #0             // X0 = 0
    ADDS X10, X0, #0x10    // X10 = 0x10 (dirección de salto)
    BR X10                 // Salta a dirección 0x10

    // Estas no deben ejecutarse si el salto funciona
    ADDS X1, X0, #1
    ADDS X2, X0, #2

.branched_here:
    ADDS X3, X0, #3
    HLT #0
