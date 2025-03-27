.section .text
.global _start

_start:
    // Inicializar X0 = 0
    ADDS X0, X0, #0

    // Sumar para obtener X0 = 0x100 (256)
    ADDS X0, X0, #0x10      // X0 = 0x10
    ADDS X0, X0, #0x10      // X0 = 0x20
    ADDS X0, X0, #0x20      // X0 = 0x40
    ADDS X0, X0, #0x40      // X0 = 0x80
    ADDS X0, X0, #0x80      // X0 = 0x100

    // X1 = X0 >> 4 → 0x100 >> 4 = 0x10
    LSR X1, X0, #4

    // Fin
    HLT #0
