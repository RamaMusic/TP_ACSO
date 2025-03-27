.section .text
.global _start

_start:
    ADDS X0, X0, #0         // X0 = 0

    ADDS X1, X0, #0xAA      // X1 = 0xAA
    ADDS X2, X0, #0x10      // X2 = 0x10

    STUR X1, [X2, #0x10]    // M[X2 + 0x10] = X1 → M[0x20] = 0xAA

    HLT #0
