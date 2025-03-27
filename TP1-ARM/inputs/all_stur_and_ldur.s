.section .text
.global _start

_start:
    // Base address: X1 = 0x10000000
    MOVZ X1, #1           // X1 = 0x1
    LSL  X1, X1, #28      // X1 = X1 << 28 = 0x10000000

    // Test values
    MOVZ X2, #0x7788      // X2 = 0x0000000000007788 (for 64-bit store)
    MOVZ X3, #0x00EF      // X3 = 0x00EF (for byte store)
    MOVZ X4, #0x1234      // X4 = 0x1234 (for halfword store)

    // Clear destination registers
    MOVZ X10, #0
    MOVZ X11, #0
    MOVZ X12, #0
    
    // Test STUR (64-bit store)
    STUR X2, [X1, #0]     // M[0x10000000] = 0x7788
    
    // Test STURB (byte store)
    STURB W3, [X1, #8]    // M[0x10000008] = 0xEF
    
    // Test STURH (halfword store)
    STURH W4, [X1, #16]   // M[0x10000010] = 0x1234
    
    // Test LDUR (64-bit load)
    LDUR X10, [X1, #0]    // X10 = M[0x10000000] = 0x7788
    
    // Test LDURB (byte load with zero extension)
    LDURB W11, [X1, #8]   // X11 = 0x000000EF
    
    // Test LDURH (halfword load with zero extension)
    LDURH W12, [X1, #16]  // X12 = 0x00001234
    
    // End test
    HLT #0
    