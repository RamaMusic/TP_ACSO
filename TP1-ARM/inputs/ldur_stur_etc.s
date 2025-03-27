.section .text
.global _start

_start:
    // Base address: X1 = 0x10000000
    MOVZ X1, #1           // X1 = 0x1
    LSL  X1, X1, #28      // X1 = X1 << 28 = 0x10000000

    // Valor de 64 bits: X2 = 0x0000000000007788
    MOVZ X2, #0x7788

    // Valor de 8 bits: X3 = 0x00EF
    MOVZ X3, #0x00EF

    // Guardar en memoria
    STUR   X2, [X1]          // M[0x10000000] = X2
    STURB  W3, [X1, #8]      // M[0x10000008] = W3(7:0)

    // Limpiar registros destino
    MOVZ X10, #0
    MOVZ X11, #0

    // Cargar desde memoria
    LDUR   X10, [X1]         // X10 = M[0x10000000]
    LDURB  W11, [X1, #8]     // X11 = M[0x10000008]

    // Fin del test
    HLT #0
