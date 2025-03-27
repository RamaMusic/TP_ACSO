#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

#include <stdint.h>
#include "shell.h"

// Estructura para representar una instrucción decodificada
typedef struct {
    uint32_t opcode;
    uint32_t opcode_31_26;
    uint32_t opcode_31_24;
    int rd;
    int rn;
    int rm;
    int64_t imm12; // Para instrucciones con inmediato
    int shift;      // Para instrucciones con extensión y shift
} Instruction;

// Definir opcodes conocidos
#define OPCODE_ADDS_IMM   0x588 // ADDS Xd, Xn, #imm
#define OPCODE_ADDS_EXT   0x558 // ADDS Xd, Xn, Xm
#define OPCODE_ADD_IMM    0x488 // ADD Xd, Xn, #imm
#define OPCODE_ADD_EXT    0x458 // ADD Xd, Xn, Xm
#define OPCODE_SUBS_IMM   0x788 // SUBS Xd, Xn, #imm
#define OPCODE_SUBS_EXT   0x758 // SUBS Xd, Xn, Xm (sin inmediato)
#define OPCODE_ANDS_REG   0x750 // ANDS Xd, Xn, Xm (Shifted Register)
#define OPCODE_EOR_REG    0x650 // EOR Xd, Xn, Xm (Shifted Register)
#define OPCODE_HLT        0x6A2 // HLT
#define OPCODE_ORR_REG    0x550 // ORR Xd, Xn, Xm (Shifted Register)
#define OPCODE_B          0x05  // B bits 31–26
#define OPCODE_BR         0x6B0 // bits 31–21 para instrucción BR
#define OPCODE_BL         0x25  // bits 31–26
#define OPCODE_B_COND     0x54  // bits 31–24
#define OPCODE_LSR_IMM    0x69A // opcode 31–21 según ensamblado real
#define OPCODE_UBFM_ALIAS 0x4B5 // bits 31–21 para UBFM (posible alias de LSL)
#define OPCODE_LSL_IMM    0xFFF // Valor especial que vamos a usar internamente
#define OPCODE_LDUR       0x7C2 // bits 31–21 para LDUR X (verificado desde el binario real)
#define OPCODE_LDURH      0x3C2
#define OPCODE_LDURB      0x1C2
#define OPCODE_STUR       0x7C0 // bits 31–21
#define OPCODE_STURB      0x1C0 // bits 31–21
#define OPCODE_STURH      0x3C0 // bits 31–21 
#define OPCODE_MOVZ       0x694 // bits 31–21 para MOVZ Xd, #imm, LSL #0 (hw = 0)
#define OPCODE_MUL        0x4D8 // bits 31-21 for MUL instruction (MADD alias)
#define OPCODE_CBZ        0x5A0 // Opcode for CBZ instruction

// Declaración de funciones
Instruction decode_instruction(uint32_t instruction);
void update_flags(int64_t result);

#endif // _INSTRUCTION_H_