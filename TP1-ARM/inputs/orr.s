.section .text
.global _start

_start:
    // Zero X0 using XZR (allowed in register-form ADDS)
    ADDS X0, XZR, XZR       // X0 = 0 (0 + 0)
    
    // Initialize X1 and X2 using X0 (now zero) as the base
    ADDS X1, X0, #0xA       // X1 = 0 + 0xA = 10
    ADDS X2, X0, #0x5       // X2 = 0 + 0x5 = 5
    
    // Perform bitwise OR
    ORR X0, X1, X2          // X0 = 0xA | 0x5 = 0xF
    
    // Compare X0 with 0xF (sets Z=1, N=0)
    CMP X0, #0xF
    
    // Copy X0 to X3 via XZR (valid register-form ADDS)
    ADDS X3, XZR, X0        // X3 = 0 + X0 = 0xF
    
    // Exit (replace HLT with syscall if needed)
    HLT #0