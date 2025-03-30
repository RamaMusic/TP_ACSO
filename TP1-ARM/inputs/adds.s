.text
// Inicialización de registros con valores pequeños
mov x0, #1                 // x0 = 1
mov x1, #2                 // x1 = 2
mov x2, #3                 // x2 = 3
mov x3, #4                 // x3 = 4

// --- ADDS con immediate (sin shift) ---
adds x4, x0, #1            // x4 = 1 + 1 = 2
adds x5, x1, #3            // x5 = 2 + 3 = 5
adds x6, x2, #2            // x6 = 3 + 2 = 5

// --- ADDS con immediate y shift permitido (LSL #12) ---
adds x7, x0, #1, LSL #12   // x7 = 1 + (1 << 12) = 4097
adds x8, x1, #2, LSL #12   // x8 = 2 + (2 << 12) = 8194

// --- ADDS con registros (sin shift) ---
adds x9, x0, x1            // x9 = 1 + 2 = 3
adds x10, x2, x3           // x10 = 3 + 4 = 7

// --- ADDS con LSL como extensión simulada ---
lsl x11, x0, #2            // x11 = 1 << 2 = 4
adds x12, x1, x11          // x12 = 2 + 4 = 6

// --- ADDS con LSR como extensión simulada ---
lsl x13, x3, #3            // x13 = 4 << 3 = 32
lsr x14, x13, #2           // x14 = 32 >> 2 = 8
adds x15, x2, x14          // x15 = 3 + 8 = 11

HLT 0
