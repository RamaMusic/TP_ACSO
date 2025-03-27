.section .text
.global _start

_start:
    // Inicialización de registros con valores de prueba
    MOVZ X1, #0x1234      // X1 = 0x0000000000001234
    MOVZ X2, #0x5678      // X2 = 0x0000000000005678
    MOVZ X3, #0x000F      // X3 = 0x000000000000000F
    // Alternativa a MOVZ X4, #0x00FF, LSL #16 sin usar LSL en MOVZ
    MOVZ X4, #0          // Inicializar X4 con 0
    MOVZ X5, #0xFF       // Load 0xFF into temporary register
    ORR X4, X4, X5       // Set least significant bits using register
    LSL X4, X4, #16      // Shift to correct position (0x00000000FFFF0000)
    
    // Limpiar registros de destino
    MOVZ X10, #0
    MOVZ X11, #0
    MOVZ X12, #0
    MOVZ X13, #0
    MOVZ X14, #0
    
    // Test ADD con dos registros
    ADD X10, X1, X2       // X10 = X1 + X2 = 0x68AC
    
    // Test ADD con inmediato de 12 bits
    ADD X11, X1, #100     // X11 = X1 + 100 = 0x1298
    
    // Test ADD con registro desplazado (LSL)
    ADD X12, X1, X3, LSL #4    // X12 = X1 + (X3 << 4) = 0x1234 + 0xF0 = 0x1324
    
    // Test ADD con registro extendido (UXTB - extender byte sin signo)
    ADD X13, X2, X3, UXTB     // X13 = X2 + (X3 extendido a byte) = 0x5678 + 0xF = 0x5687
    
    // Test ADD con registro extendido y desplazamiento
    ADD X14, X1, X3, UXTB #2  // X14 = X1 + (X3 extendido a byte y desplazado 2) = 0x1234 + (0xF << 2) = 0x1234 + 0x3C = 0x1270
    
    // Test ADD con valores grandes
    ADD X15, X4, X2       // X15 = X4 + X2 = 0x00FF0000 + 0x5678 = 0x00FF5678
    
    // Fin de la prueba
    HLT #0
    