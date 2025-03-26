.section .text
.global _start

_start:
    // X0 = 0 (registro base)
    ADDS X0, X0, #0

    // X1 = 5, X2 = 5, X3 = 10
    ADDS X1, X0, #5
    ADDS X2, X0, #5
    ADDS X3, X0, #10

    // ----------- BEQ: Z == 1 ------------
    CMP X1, X2          // Z = 1
    BEQ salto_eq        // Debería saltar
    ADDS X10, X0, #10   // NO debe ejecutarse si el salto funciona

salto_eq:
    ADDS X11, X0, #11   // Se ejecuta si BEQ funcionó

    // ----------- BNE: Z == 0 ------------
    CMP X1, X3          // Z = 0
    BNE salto_ne
    ADDS X12, X0, #12

salto_ne:
    ADDS X13, X0, #13

    // ----------- BGT: Z == 0 && N == 0 ------------
    CMP X3, X1          // X3 > X1 → Z = 0, N = 0
    BGT salto_gt
    ADDS X14, X0, #14

salto_gt:
    ADDS X15, X0, #15

    // ----------- BLT: N == 1 ------------
    CMP X1, X3          // X1 < X3 → N = 1
    BLT salto_lt
    ADDS X16, X0, #16

salto_lt:
    ADDS X17, X0, #17

    // ----------- BGE: N == 0 ------------
    CMP X3, X1          // X3 > X1 → N = 0
    BGE salto_ge
    ADDS X18, X0, #18

salto_ge:
    ADDS X19, X0, #19
    
    // ----------- BLE: Z == 1 || N == 1 ------------
    CMP X1, X3          // X1 < X3 → N = 1
    BLE salto_le
    ADDS X20, X0, #20   // Esta línea debe estar bien escrita

salto_le:
    ADDS X21, X0, #21

    HLT #0
