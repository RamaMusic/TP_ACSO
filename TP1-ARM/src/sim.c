#include <stdio.h>
#include <stdint.h>
#include "shell.h"

// Declaraciones explícitas si no están en shell.h
extern CPU_State CURRENT_STATE, NEXT_STATE;
extern uint32_t mem_read_32(uint64_t address);

// Estructura para representar una instrucción decodificada
typedef struct {
    uint32_t opcode;
    int rd;
    int rn;
    int rm;
    int64_t imm12; // Para instrucciones con inmediato
    int shift;      // Para instrucciones con extensión y shift
} Instruction;

// Definir opcodes conocidos
#define OPCODE_ADDS_IMM   0x588  // ADDS Xd, Xn, #imm
// #define OPCODE_ADDS_EXT   0x5B0  // ADDS Xd, Xn, Xm, extend shift
#define OPCODE_ADDS_EXT    0x558  // ADD Xd, Xn, Xm
#define OPCODE_SUBS_IMM   0x788  // SUBS Xd, Xn, #imm
// #define OPCODE_SUBS_EXT   0x5F0  // SUBS Xd, Xn, Xm, extend shift
#define OPCODE_SUBS_EXT   0x758  // SUBS Xd, Xn, Xm (sin inmediato)
// #define OPCODE_CMP_IMM    0x7C8  // CMP Xn, #imm (Mismo opcode que SUBS IMM pero con XZR)
// #define OPCODE_CMP_EXT    0x758  // CMP Xn, Xm (Mismo opcode que SUBS REG pero con XZR)
#define OPCODE_HLT        0x6A2  // HLT

// Función para decodificar una instrucción
Instruction decode_instruction(uint32_t instruction) {
    Instruction inst;
    inst.opcode = (instruction >> 21) & 0x7FF;  // Extraer bits 31-21
    inst.rd = (instruction >> 0) & 0x1F;        // Extraer bits 4-0 (Registro destino)
    inst.rn = (instruction >> 5) & 0x1F;        // Extraer bits 9-5 (Registro fuente 1)
    inst.rm = (instruction >> 16) & 0x1F;       // Extraer bits 20-16 (Registro fuente 2, solo en EXT y REG)
    inst.shift = (instruction >> 22) & 0x3;     // Extraer bits 23-22 (Shift en IMM)
    
    if (inst.opcode == OPCODE_ADDS_IMM || inst.opcode == OPCODE_SUBS_IMM) {
        inst.imm12 = (instruction >> 10) & 0xFFF; // Extraer bits 21-10 (valor inmediato)
        if (inst.shift == 1) {
            inst.imm12 = inst.imm12 << 12;  // Si shift == 01, mover imm12 12 bits a la izquierda
        }
    } else {
        inst.imm12 = 0;
    }
    return inst;
}

// Función para actualizar los flags
void update_flags(int64_t result) {
    NEXT_STATE.FLAG_Z = (result == 0) ? 1 : 0;
    NEXT_STATE.FLAG_N = (result < 0) ? 1 : 0;
}

void process_instruction() {
    // 1️⃣ Leer la instrucción desde la memoria
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    
    // 2️⃣ Decodificar la instrucción
    Instruction inst = decode_instruction(instruction);
    
    // 3️⃣ Mostrar la instrucción decodificada (para pruebas)
    printf("PC: 0x%08lx | Instrucción: 0x%08x\n", CURRENT_STATE.PC, instruction);
    printf("Opcode: 0x%03x | Rd: X%d | Rn: X%d | Rm: X%d | Imm12: %ld | Shift: %d\n", 
            inst.opcode, inst.rd, inst.rn, inst.rm, inst.imm12, inst.shift);
    
    // 4️⃣ Ejecutar la instrucción
    switch (inst.opcode) {
        case OPCODE_ADDS_IMM:
            NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] + inst.imm12;
            update_flags(NEXT_STATE.REGS[inst.rd]);
            printf("Ejecutando ADDS (IMM): X%d = X%d + %ld | Flags -> Z: %d, N: %d\n", 
                    inst.rd, inst.rn, inst.imm12, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            break;

        case OPCODE_ADDS_EXT:
            NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] + CURRENT_STATE.REGS[inst.rm];
            update_flags(NEXT_STATE.REGS[inst.rd]);
            printf("Ejecutando ADDS (EXT): X%d = X%d + X%d | Flags -> Z: %d, N: %d\n", 
                    inst.rd, inst.rn, inst.rm, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            break;
        
        case OPCODE_SUBS_IMM:
            if (inst.rd == 31) { // Si Rd es XZR, tratar como CMP
                update_flags(CURRENT_STATE.REGS[inst.rn] - inst.imm12);
                printf("Ejecutando CMP (IMM): XZR = X%d - %ld | Flags -> Z: %d, N: %d", 
                        inst.rn, inst.imm12, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            } else {
                NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] - inst.imm12;
                update_flags(NEXT_STATE.REGS[inst.rd]);
                printf("Ejecutando SUBS (IMM): X%d = X%d - %ld | Flags -> Z: %d, N: %d", 
                        inst.rd, inst.rn, inst.imm12, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            }
            break;

        case OPCODE_SUBS_EXT:
            if (inst.rd == 31) { // Si Rd es XZR, tratar como CMP
                update_flags(CURRENT_STATE.REGS[inst.rn] - CURRENT_STATE.REGS[inst.rm]);
                printf("Ejecutando CMP (EXT): XZR = X%d - X%d | Flags -> Z: %d, N: %d", 
                        inst.rn, inst.rm, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            } else {
                NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] - CURRENT_STATE.REGS[inst.rm];
                update_flags(NEXT_STATE.REGS[inst.rd]);
                printf("Ejecutando SUBS (EXT): X%d = X%d - X%d | Flags -> Z: %d, N: %d", 
                        inst.rd, inst.rn, inst.rm, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            }
            break;
        
        case OPCODE_HLT:
            printf("Deteniendo la simulación (HLT)\n");
            RUN_BIT = 0;
            return;
        
        default:
            printf("Instrucción no reconocida (Opcode: 0x%03x)\n", inst.opcode);
    }
    
    // 5️⃣ Avanzar el PC a la siguiente instrucción
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    
    // 6️⃣ Actualizar el estado del CPU
    CURRENT_STATE = NEXT_STATE;
}
