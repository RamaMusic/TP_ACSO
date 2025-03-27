.section .text
.global _start

_start:
    // Load first operand: X1 = 5
    MOVZ X1, #5

    // Load second operand: X2 = 7
    MOVZ X2, #7

    // Multiply: X0 = X1 * X2 (should be 35)
    MUL X0, X1, X2

    // End test
    HLT #0
