.section .text
.global _start

_start:
    // Initialize registers with test values
    MOVZ X1, #0          // X1 = 0
    MOVZ X2, #5          // X2 = 5

    // Test CBZ with X1 (should branch as X1 = 0)
    CBZ X1, zero_case
    MOVZ X10, #1         // Should be skipped
    
zero_case:
    MOVZ X10, #42        // X10 = 42

    // Test CBNZ with X2 (should branch as X2 ≠ 0)
    CBNZ X2, nonzero_case
    MOVZ X11, #2         // Should be skipped

nonzero_case:
    MOVZ X11, #84        // X11 = 84

    // End test
    HLT #0
