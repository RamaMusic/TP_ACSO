#include <stdio.h>
#include <stdint.h>
#include "instruction.h"
#include "shell.h"

// Función para decodificar una instrucción
Instruction decode_instruction(uint32_t instruction) {
    Instruction inst;
    inst.opcode = (instruction >> 21) & 0x7FF;  // Extraer bits 31-21
    inst.opcode_31_26 = (instruction >> 26);    // Extraer bits 31-26
    inst.opcode_31_24 = (instruction >> 24);    // Extraer bits 31-24
    inst.rd = (instruction >> 0) & 0x1F;        // Extraer bits 4-0 (Registro destino)
    inst.rn = (instruction >> 5) & 0x1F;        // Extraer bits 9-5 (Registro fuente 1)
    inst.rm = (instruction >> 16) & 0x1F;       // Extraer bits 20-16 (Registro fuente 2, solo en EXT y REG)
    inst.shift = (instruction >> 22) & 0x3;     // Extraer bits 23-22 (Shift en IMM)
    inst.imm12 = 0;
    
    if (inst.opcode == OPCODE_ADDS_IMM || inst.opcode == OPCODE_SUBS_IMM || inst.opcode == OPCODE_ADD_IMM) {
        inst.imm12 = (instruction >> 10) & 0xFFF; // Bits 21-10
    
        switch (inst.shift) {
            case 0b00: // No shift
                break;
            case 0b01: // LSL #12
                inst.imm12 <<= 12;
                break;
            default: // 0b10 y 0b11 son inválidos para esta instrucción
                printf("Shift inválido en instrucción ADDS/SUBS/ADD (IMM): %d\n", inst.shift);
                break;
        }
    }

    // LSR
    if (inst.opcode == 0x69B) {  // UBFM
        uint8_t immr = (instruction >> 16) & 0x3F;
        uint8_t imms = (instruction >> 10) & 0x3F;

        if (imms != 0b111111 && (imms + 1) == immr) {
            inst.opcode = OPCODE_LSL_IMM;
            inst.imm12 = 64 - immr;  // shift = 64 - immr
        }
    }

    if (inst.opcode == OPCODE_LSR_IMM) {
        inst.rd = instruction & 0x1F;              // bits 4–0
        inst.rn = (instruction >> 5) & 0x1F;       // bits 9–5
        inst.imm12 = (instruction >> 16) & 0x3F;   // bits 21–16: shift amount
    }

    // Instrucción B: opcode está en bits 31:26
    if (inst.opcode_31_26 == OPCODE_B) {
        inst.opcode = OPCODE_B;
        int32_t imm26 = instruction & 0x03FFFFFF;  // Bits 25-0

        // Sign-extend: si bit 25 está en 1, rellenamos con 1s arriba
        if (imm26 & (1 << 25)) {
            imm26 |= 0xFC000000; // Rellenar bits 31–26 con 1s (sign extension)
        }

        inst.imm12 = ((int64_t)imm26) << 2; // Multiplicar por 4 (agregar :'00')
    }

    if (inst.opcode_31_26 == OPCODE_BL) {
        inst.opcode = OPCODE_BL;
        int32_t imm26 = instruction & 0x03FFFFFF;

        if (imm26 & (1 << 25)) {
            imm26 |= 0xFC000000;  // Sign extend
        }

        inst.imm12 = ((int64_t)imm26) << 2;
        return inst;
    }

    if (inst.opcode_31_24 == OPCODE_B_COND) {
        inst.opcode = OPCODE_B_COND;
    
        int32_t imm19 = (instruction >> 5) & 0x7FFFF; // bits 23–5
        if (imm19 & (1 << 18)) {
            imm19 |= 0xFFF80000;  // Sign extend 19 bits
        }
    
        inst.imm12 = ((int64_t)imm19) << 2;  // offset real
        inst.rd = instruction & 0xF;         // bits 3–0 → condición
        return inst;
    }

    if (((instruction >> 21) & 0x7FF) == OPCODE_LDUR) {
        inst.opcode = OPCODE_LDUR;
        inst.rd = instruction & 0x1F;              // Rt
        inst.rn = (instruction >> 5) & 0x1F;       // Rn
        int32_t imm9 = (instruction >> 12) & 0x1FF;
        // Sign extension de 9 bits a 64 bits
        if (imm9 & (1 << 8)) {
            imm9 |= ~0x1FF;
        }
        inst.imm12 = imm9;
    }
    
    if (((instruction >> 21) & 0x7FF) == OPCODE_LDURH) {
        inst.opcode = OPCODE_LDURH;
        inst.rd = instruction & 0x1F;
        inst.rn = (instruction >> 5) & 0x1F;
        int32_t imm9 = (instruction >> 12) & 0x1FF;
        if (imm9 & (1 << 8)) {
            imm9 |= ~0x1FF;
        }
        inst.imm12 = imm9;
    }
    
    if (((instruction >> 21) & 0x7FF) == OPCODE_LDURB) {
        inst.opcode = OPCODE_LDURB;
        inst.rd = instruction & 0x1F;
        inst.rn = (instruction >> 5) & 0x1F;
        int32_t imm9 = (instruction >> 12) & 0x1FF;
        if (imm9 & (1 << 8)) {
            imm9 |= ~0x1FF;
        }
        inst.imm12 = imm9;
    }
    if (inst.opcode == OPCODE_STUR) {
        int32_t imm9 = (instruction >> 12) & 0x1FF;  // bits 20–12
        if (imm9 & (1 << 8)) {
            imm9 |= 0xFFFFFE00;  // Sign-extend 9 bits
        }
    
        inst.imm12 = imm9;
        inst.rn = (instruction >> 5) & 0x1F;   // base register
        inst.rd = instruction & 0x1F;          // Rt = registro a almacenar
    }

    if (inst.opcode == OPCODE_STURB) {
        int32_t imm9 = (instruction >> 12) & 0x1FF; // bits 20–12
        if (imm9 & (1 << 8)) {
            imm9 |= 0xFFFFFE00;  // Sign-extend a 64 bits
        }
    
        inst.imm12 = imm9;
        inst.rn = (instruction >> 5) & 0x1F;   // base register
        inst.rd = instruction & 0x1F;          // Rt = valor a guardar
    }

    if (inst.opcode == OPCODE_STURH) {
        int32_t imm9 = (instruction >> 12) & 0x1FF; // bits 20–12
        if (imm9 & (1 << 8)) {
            imm9 |= 0xFFFFFE00;  // Sign-extend a 64 bits
        }
    
        inst.imm12 = imm9;
        inst.rn = (instruction >> 5) & 0x1F;   // base register
        inst.rd = instruction & 0x1F;          // Rt = valor a guardar
    }
    
    if (inst.opcode == OPCODE_MOVZ) {
        inst.imm12 = (instruction >> 5) & 0xFFFF;    // bits 20–5 → imm16
        uint8_t hw = (instruction >> 21) & 0x3;      // bits 22–21
        if (hw != 0) {
            printf("MOVZ solo implementado con hw = 0\n");
        }
    }

    if (((instruction >> 21) & 0x7FF) == OPCODE_MUL) {
        inst.opcode = OPCODE_MUL;
        inst.rd = instruction & 0x1F;              // bits 4-0: Rd
        inst.rn = (instruction >> 5) & 0x1F;       // bits 9-5: Rn (first source)
        inst.rm = (instruction >> 16) & 0x1F;      // bits 20-16: Rm (second source)
    }

    
    if ((inst.opcode_31_24 & 0xFE) == 0xB4) {  // Base opcode for both CBZ/CBNZ
        inst.opcode = OPCODE_CBZ;  // We'll use CBZ as the base opcode
        inst.rd = instruction & 0x1F;  // bits 4-0: Rt
        int32_t imm19 = (instruction >> 5) & 0x7FFFF;  // bits 23-5
        
        // Store if it's CBNZ in the shift field (bit 24 differentiates them)
        inst.shift = (instruction >> 24) & 0x1;  // 0 for CBZ, 1 for CBNZ
        
        // Sign extend imm19
        if (imm19 & (1 << 18)) {
            imm19 |= 0xFFF80000;
        }
        
        inst.imm12 = ((int64_t)imm19) << 2;  // multiply by 4 for byte offset
    }
    
    return inst;
}

// Función para actualizar los flags
void update_flags(int64_t result) {
    extern CPU_State NEXT_STATE;
    NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
    NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
}