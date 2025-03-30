.section .text
.global _start

_start:
    // X0 = 0 (registro base)
    ADDS X0, X0, #0

    // X1 = 5, X2 = 5, X3 = 10, X4 = 3
    ADDS X1, X0, #5
    ADDS X2, X0, #5
    ADDS X3, X0, #10
    ADDS X4, X0, #3

    // ----------- BEQ: Z == 1 ------------
    CMP X1, X2          // 5 == 5 → Z = 1
    BEQ salto_eq        // Debe saltar
    ADDS X10, X0, #10   // No debe ejecutarse

salto_eq:
    ADDS X11, X0, #11   // Se ejecuta si BEQ funcionó

    // ----------- BNE: Z == 0 ------------
    CMP X1, X3          // 5 != 10 → Z = 0
    BNE salto_ne
    ADDS X12, X0, #12   // No debe ejecutarse

salto_ne:
    ADDS X13, X0, #13   // Se ejecuta si BNE funcionó

    // ----------- BGT: Z == 0 && N == 0 ------------
    CMP X3, X1          // 10 > 5 → Z = 0, N = 0
    BGT salto_gt
    ADDS X14, X0, #14   // No debe ejecutarse

salto_gt:
    ADDS X15, X0, #15   // Se ejecuta si BGT funcionó

    // ----------- BLT: N == 1 ------------
    CMP X1, X3          // 5 < 10 → N = 1
    BLT salto_lt
    ADDS X16, X0, #16   // No debe ejecutarse

salto_lt:
    ADDS X17, X0, #17   // Se ejecuta si BLT funcionó

    // ----------- BGE: N == 0 ------------
    CMP X3, X1          // 10 >= 5 → N = 0
    BGE salto_ge
    ADDS X18, X0, #18   // No debe ejecutarse

salto_ge:
    ADDS X19, X0, #19   // Se ejecuta si BGE funcionó

    // ----------- BLE: Z == 1 || N == 1 ------------
    CMP X1, X3          // 5 <= 10 → N = 1
    BLE salto_le
    ADDS X20, X0, #20   // No debe ejecutarse

salto_le:
    ADDS X21, X0, #21   // Se ejecuta si BLE funcionó

    HLT #0
