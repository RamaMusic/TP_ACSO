.text
// Inicialización de registros con valores pequeños
mov x0, #10                // x0 = 10
mov x1, #4                 // x1 = 4
mov x2, #6                 // x2 = 6
mov x3, #2                 // x3 = 2

// --- SUBS con immediate (sin shift) ---
subs x4, x0, #1            // x4 = 10 - 1 = 9
subs x5, x1, #3            // x5 = 4 - 3 = 1
subs x6, x2, #2            // x6 = 6 - 2 = 4

// --- SUBS con immediate y shift permitido (LSL #12) ---
subs x7, x0, #1, LSL #12   // x7 = 10 - (1 << 12) = 10 - 4096 = -4086
subs x8, x2, #2, LSL #12   // x8 = 6 - (2 << 12) = 6 - 8192 = -8186

// --- SUBS con registros (sin shift) ---
subs x9, x0, x1            // x9 = 10 - 4 = 6
subs x10, x2, x3           // x10 = 6 - 2 = 4

// --- SUBS con LSL como extensión simulada ---
lsl x11, x3, #2            // x11 = 2 << 2 = 8
subs x12, x0, x11          // x12 = 10 - 8 = 2

// --- SUBS con LSR como extensión simulada ---
lsl x13, x2, #3            // x13 = 6 << 3 = 48
lsr x14, x13, #2           // x14 = 48 >> 2 = 12
subs x15, x0, x14          // x15 = 10 - 12 = -2

HLT 0
