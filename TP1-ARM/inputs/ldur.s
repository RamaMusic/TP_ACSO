.section .text
.global _start

_start:
    // X2 = 0
    ADDS X2, X2, #0        // Limpia X2

    // X2 += 0x10000000
    // Se logra con immediate #1 y shift 28 (1 << 28 = 0x10000000)
    ADDS X2, X2, #1, LSL #28

    // X2 += 0x10 → X2 = 0x10000010
    ADDS X2, X2, #0x10

    // LDUR X1, [X2, #0] → carga desde 0x10000010
    LDUR X1, [X2, #0]

    // Fin del programa
    HLT #0
