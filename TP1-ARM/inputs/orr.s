.text
// Inicialización básica
mov x10, #1               // x10 = 0b0001
mov x11, #2               // x11 = 0b0010
orr x0, x10, x11          // x0 = 0b0001 | 0b0010 = 0b0011 (3)

// Otro caso
mov x12, #0               // x12 = 0b0000
mov x13, #8               // x13 = 0b1000
orr x1, x12, x13          // x1 = 0b0000 | 0b1000 = 0b1000 (8)

// Otro caso
mov x14, #5               // x14 = 0b0101
mov x15, #10              // x15 = 0b1010
orr x2, x14, x15          // x2 = 0b0101 | 0b1010 = 0b1111 (15)

// OR con sí mismo
orr x3, x10, x10          // x3 = 0b0001 | 0b0001 = 0b0001 (1)

HLT 0
