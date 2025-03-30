.section .text
.global _start

_start:
    // Inicialización base: X0 = 0
    mov x0, #0

    // --- ADD con inmediato (sin shift) ---
    mov x1, #5                 // x1 = 5
    add x2, x1, #3             // x2 = 5 + 3 = 8

    // --- ADD con inmediato y shift LSL #12 ---
    // x3 = 5 + (1 << 12) = 5 + 4096 = 4101
    add x3, x1, #1, LSL #12

    // --- ADD con registros (Extended Register) ---
    mov x4, #10                // x4 = 10
    mov x5, #20                // x5 = 20
    add x6, x4, x5             // x6 = 10 + 20 = 30

    // Otro caso de registro para verificar suma con cero
    mov x7, #0
    add x8, x4, x7             // x8 = 10 + 0 = 10

    // Registro con resultado más grande
    mov x9, #100
    add x10, x9, x5            // x10 = 100 + 20 = 120

    HLT #0
