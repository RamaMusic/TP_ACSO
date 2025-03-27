#include <stdio.h>
#include <stdint.h>
#include "shell.h"
#include "instruction.h"
#include "executor.h"

// Declaraciones explícitas
extern CPU_State CURRENT_STATE, NEXT_STATE;
extern uint32_t mem_read_32(uint64_t address);

/**
 * Función principal que procesa una instrucción ARM.
 * Esta función es llamada por el simulador en cada ciclo.
 */
void process_instruction() {
    // 1️⃣ Leer la instrucción desde la memoria
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);
    
    // 2️⃣ Decodificar la instrucción
    Instruction inst = decode_instruction(instruction);
    
    // 3️⃣ Mostrar la instrucción decodificada (para depuración)
    printf("PC: 0x%08lx | Instrucción: 0x%08x\n", CURRENT_STATE.PC, instruction);
    printf("Opcode: 0x%03x | Rd: X%d | Rn: X%d | Rm: X%d | Imm12: %ld | Shift: %d\n", 
            inst.opcode, inst.rd, inst.rn, inst.rm, inst.imm12, inst.shift);
    
    // 4️⃣ Ejecutar la instrucción según su tipo
    int branch_taken = 0;
    int inst_type = get_instruction_type(inst.opcode);
    
    switch (inst_type) {
        case 0: // Aritmética/Lógica
            execute_arithmetic_logic(inst);
            break;
            
        case 1: // Memoria
            execute_memory(inst);
            break;
            
        case 2: // Salto
            branch_taken = execute_branch(inst);
            break;
            
        default:
            printf("Tipo de instrucción desconocido (Opcode: 0x%03x)\n", inst.opcode);
    }
    
    // 5️⃣ Avanzar el PC a la siguiente instrucción si no se tomó un salto
    if (!branch_taken) {
        NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    }
    
    // 6️⃣ Actualizar el estado del CPU
    CURRENT_STATE = NEXT_STATE;
}