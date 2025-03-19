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
    int64_t imm12; // Para instrucciones con inmediato
} Instruction;

// Definir opcodes conocidos
#define OPCODE_ADDS_IMM  0x588  // ADDS Xd, Xn, #imm (Corrección de opcode)
#define OPCODE_HLT       0x7E0  // HLT

// Función para decodificar una instrucción
Instruction decode_instruction(uint32_t instruction) {
    Instruction inst;
    inst.opcode = (instruction >> 21) & 0x7FF;  // Extraer bits 31-21
    inst.rd = (instruction >> 0) & 0x1F;        // Extraer bits 4-0 (Registro destino)
    inst.rn = (instruction >> 5) & 0x1F;        // Extraer bits 9-5 (Registro fuente 1)
    
    if (inst.opcode == OPCODE_ADDS_IMM) {
        inst.imm12 = (instruction >> 10) & 0xFFF; // Extraer bits 21-10 (valor inmediato)
    } else {
        inst.imm12 = 0;
    }
    return inst;
}

void process_instruction() {
    // 1️⃣ Leer la instrucción desde la memoria
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    
    // 2️⃣ Decodificar la instrucción
    Instruction inst = decode_instruction(instruction);
    
    // 3️⃣ Mostrar la instrucción decodificada (para pruebas)
    printf("PC: 0x%08lx | Instrucción: 0x%08x\n", CURRENT_STATE.PC, instruction);
    printf("Opcode: 0x%03x | Rd: X%d | Rn: X%d | Imm12: %ld\n", inst.opcode, inst.rd, inst.rn, inst.imm12);
    
    // 4️⃣ Ejecutar la instrucción
    switch (inst.opcode) {
        case OPCODE_ADDS_IMM:
            NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] + inst.imm12;
            printf("Ejecutando ADDS: X%d = X%d + %ld\n", inst.rd, inst.rn, inst.imm12);
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
