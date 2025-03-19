.section .text
.global _start

_start:
    ADDS X0, X10, #1    // x0 = 0x1
    ADDS X1, X11, #11   // x1 = 0xb
    CMP X0, X1          // Comparación, debería dar flag_n = 1
    ADDS X3, XZR, X1    // Solo para visualizar que CMP se ejecuta
    CMP X1, X0          // Comparación, ahora flag_n = 0
    ADDS X4, X12, #1    // x4 = 0x1
    CMP X0, X4          // Comparación, ahora flag_n = 0 y flag_z = 1

    HLT #0
