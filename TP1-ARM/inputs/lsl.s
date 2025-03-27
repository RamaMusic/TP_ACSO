.section .text
.global _start

_start:
    ADDS X0, X0, #5     // X0 = 0 + 5 → 5
    LSL X1, X0, #3       // X1 = X0 << 3 → 5 << 3 = 40
    ADDS X2, X1, #2      // X2 = X1 + 2 → 42
    HLT #0
