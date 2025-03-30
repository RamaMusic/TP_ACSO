#include <stdio.h>
#include <stdint.h>
#include "instruction.h"
#include "shell.h"

// Declaraciones explícitas
extern CPU_State CURRENT_STATE, NEXT_STATE;
extern int RUN_BIT;
extern uint32_t mem_read_32(uint64_t address);
extern void mem_write_32(uint64_t address, uint32_t value);

// Función para ejecutar instrucciones aritméticas y lógicas
void execute_arithmetic_logic(Instruction inst) {
    switch (inst.opcode) {
        case OPCODE_ADD_IMM:
            NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] + inst.imm12;
            printf("Ejecutando ADD (IMM): X%d = X%d + %ld\n", 
                    inst.rd, inst.rn, inst.imm12);
            break;
            
        case OPCODE_ADD_EXT:
            NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] + CURRENT_STATE.REGS[inst.rm];
            printf("Ejecutando ADD (EXT): X%d = X%d + X%d%s%d\n", 
                    inst.rd, inst.rn, inst.rm, 
                    (inst.shift > 0) ? ", LSL #" : "", 
                    (inst.shift > 0) ? inst.shift : 0);
            break;
            
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
                printf("Ejecutando CMP (IMM): XZR = X%d - %ld | Flags -> Z: %d, N: %d\n", 
                        inst.rn, inst.imm12, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            } else {
                NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] - inst.imm12;
                update_flags(NEXT_STATE.REGS[inst.rd]);
                printf("Ejecutando SUBS (IMM): X%d = X%d - %ld | Flags -> Z: %d, N: %d\n", 
                        inst.rd, inst.rn, inst.imm12, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            }
            break;

        case OPCODE_SUBS_EXT:
            if (inst.rd == 31) { // Si Rd es XZR, tratar como CMP
                update_flags(CURRENT_STATE.REGS[inst.rn] - CURRENT_STATE.REGS[inst.rm]);
                printf("Ejecutando CMP (EXT): XZR = X%d - X%d | Flags -> Z: %d, N: %d\n", 
                        inst.rn, inst.rm, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            } else {
                NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] - CURRENT_STATE.REGS[inst.rm];
                update_flags(NEXT_STATE.REGS[inst.rd]);
                printf("Ejecutando SUBS (EXT): X%d = X%d - X%d | Flags -> Z: %d, N: %d\n", 
                        inst.rd, inst.rn, inst.rm, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            }
            break;

        case OPCODE_ANDS_REG:
            NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] & CURRENT_STATE.REGS[inst.rm];
            update_flags(NEXT_STATE.REGS[inst.rd]);
            printf("Ejecutando ANDS (REG): X%d = X%d & X%d | Flags -> Z: %d, N: %d\n", 
                    inst.rd, inst.rn, inst.rm, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            break;

        case OPCODE_EOR_REG:
            NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] ^ CURRENT_STATE.REGS[inst.rm];
            printf("Ejecutando EOR (REG): X%d = X%d ^ X%d | Flags -> Z: %d, N: %d\n", 
                    inst.rd, inst.rn, inst.rm, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            break;

        case OPCODE_ORR_REG:
            if (inst.rn == 31) {
                // MOV alias: ORR Xd, XZR, Xm → Xd = Xm
                NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rm];
                printf("Ejecutando MOV (alias ORR): X%d = X%d\n", inst.rd, inst.rm);
            } else {
                NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] | CURRENT_STATE.REGS[inst.rm];
                update_flags(NEXT_STATE.REGS[inst.rd]);
                printf("Ejecutando ORR (REG): X%d = X%d | X%d | Flags -> Z: %d, N: %d\n",
                       inst.rd, inst.rn, inst.rm, NEXT_STATE.FLAG_Z, NEXT_STATE.FLAG_N);
            }
            break;

        case INT_OPCODE_LSL:
            NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] << inst.imm12;
            printf("Ejecutando LSL (IMM): X%d = X%d << %ld\n",
                inst.rd, inst.rn, inst.imm12);
            break;

        case INT_OPCODE_LSR:
            NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] >> inst.imm12;
            printf("Ejecutando LSR (IMM): X%d = X%d >> %ld → 0x%lx\n",
                inst.rd, inst.rn, inst.imm12, NEXT_STATE.REGS[inst.rd]);
            break;

        case OPCODE_MOVZ:
            NEXT_STATE.REGS[inst.rd] = inst.imm12 & 0xFFFF;
            printf("Ejecutando MOVZ: X%d = 0x%lx\n", inst.rd, NEXT_STATE.REGS[inst.rd]);
            break;

        case OPCODE_MUL: {
            NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] * CURRENT_STATE.REGS[inst.rm];
            printf("Ejecutando MUL: X%d = X%d * X%d = 0x%lx\n",
                   inst.rd, inst.rn, inst.rm, NEXT_STATE.REGS[inst.rd]);
            break;
        }

        case OPCODE_HLT:
            printf("Deteniendo la simulación (HLT)\n");
            RUN_BIT = 0;
            break;

        default:
            printf("Instrucción aritmética/lógica no reconocida (Opcode: 0x%03x)\n", inst.opcode);
    }
}

// Función para ejecutar instrucciones de memoria
void execute_memory(Instruction inst) {
    switch (inst.opcode) {
        case OPCODE_LDUR: {
            uint64_t addr = CURRENT_STATE.REGS[inst.rn] + inst.imm12;
            if (addr < 0x10000000) {
                printf("LDUR: dirección fuera de la memoria (0x%lx)\n", addr);
                break;
            }
            uint64_t val = 0;
            val |= mem_read_32(addr);
            val |= ((uint64_t)mem_read_32(addr + 4)) << 32;
            NEXT_STATE.REGS[inst.rd] = val;
            printf("Ejecutando LDUR: X%d = M[X%d + %ld] = 0x%lx\n",
                    inst.rd, inst.rn, inst.imm12, val);
            break;
        }

        case OPCODE_LDURH: {
            uint64_t addr = CURRENT_STATE.REGS[inst.rn] + inst.imm12;
            if (addr < 0x10000000) {
                printf("LDURH: dirección fuera de la memoria (0x%lx)\n", addr);
                break;
            }
            uint32_t word = mem_read_32(addr);
            uint64_t half = word & 0xFFFF;  // Extraer solo los primeros 16 bits
            NEXT_STATE.REGS[inst.rd] = half;
            printf("Ejecutando LDURH: X%d = zero_extend_16(M[X%d + %ld]) = 0x%lx\n",
                   inst.rd, inst.rn, inst.imm12, half);
            break;
        }

        case OPCODE_LDURB: {
            uint64_t addr = CURRENT_STATE.REGS[inst.rn] + inst.imm12;
            if (addr < 0x10000000) {
                printf("LDURB: dirección fuera de la memoria (0x%lx)\n", addr);
                break;
            }
            uint32_t word = mem_read_32(addr);
            uint64_t byte = word & 0xFF;  // Extraer solo los primeros 8 bits
            NEXT_STATE.REGS[inst.rd] = byte;
            printf("Ejecutando LDURB: X%d = zero_extend_8(M[X%d + %ld]) = 0x%lx\n",
                   inst.rd, inst.rn, inst.imm12, byte);
            break;
        }        
            
        case OPCODE_STUR: {
            uint64_t addr = CURRENT_STATE.REGS[inst.rn] + inst.imm12;
            uint64_t data = CURRENT_STATE.REGS[inst.rd];
            mem_write_32(addr, data & 0xFFFFFFFF);         // parte baja
            mem_write_32(addr + 4, (data >> 32) & 0xFFFFFFFF); // parte alta
            printf("Ejecutando STUR: M[0x%08lx] = X%d (0x%016lx)\n",
                    addr, inst.rd, data);
            break;
        }
            
        case OPCODE_STURB: {
            uint64_t addr = CURRENT_STATE.REGS[inst.rn] + inst.imm12;
            uint8_t byte = CURRENT_STATE.REGS[inst.rd] & 0xFF;
            mem_write_32(addr, (mem_read_32(addr) & 0xFFFFFF00) | byte);
            printf("Ejecutando STURB: M[0x%08lx] = X%d(7:0) → 0x%02x\n", addr, inst.rd, byte);
            break;
        }

        case OPCODE_STURH: {
            uint64_t addr = CURRENT_STATE.REGS[inst.rn] + inst.imm12;
            uint16_t half = CURRENT_STATE.REGS[inst.rd] & 0xFFFF;
            mem_write_32(addr, (mem_read_32(addr) & 0xFFFF0000) | half);
            printf("Ejecutando STURH: M[0x%08lx] = X%d(15:0) → 0x%04x\n", addr, inst.rd, half);
            break;
        }

        default:
            printf("Instrucción de memoria no reconocida (Opcode: 0x%03x)\n", inst.opcode);
    }
}

// Función para ejecutar instrucciones de salto
int execute_branch(Instruction inst) {
    int branch_taken = 0;
    
    switch (inst.opcode) {
        case OPCODE_B:
            NEXT_STATE.PC = CURRENT_STATE.PC + inst.imm12;
            printf("Ejecutando B: salto a PC + %ld → 0x%08lx\n", inst.imm12, NEXT_STATE.PC);
            branch_taken = 1;
            break;

        case OPCODE_BR:
            NEXT_STATE.PC = CURRENT_STATE.REGS[inst.rn];
            printf("Ejecutando BR: salto a dirección contenida en X%d → 0x%08lx\n",
                   inst.rn, NEXT_STATE.PC);
            branch_taken = 1;
            break;

        case OPCODE_BL:
            NEXT_STATE.REGS[30] = CURRENT_STATE.PC + 4; // Guardar dirección de retorno
            NEXT_STATE.PC = CURRENT_STATE.PC + inst.imm12; // Saltar
            printf("Ejecutando BL: salto a PC + %ld → 0x%08lx | X30 (LR) = 0x%08lx\n",
                   inst.imm12, NEXT_STATE.PC, NEXT_STATE.REGS[30]);
            branch_taken = 1;
            break;

        case OPCODE_B_COND: {
            int cond = inst.rd; // Esto son solo los ulitmos 3 bits en este caso.
            int take_branch = 0;

            switch (cond) {
                case 0x0: // EQ: Z == 1
                    take_branch = CURRENT_STATE.FLAG_Z == 1;
                    printf("B.EQ: Z=%d ", CURRENT_STATE.FLAG_Z);
                    break;
                case 0x1: // NE: Z == 0
                    take_branch = CURRENT_STATE.FLAG_Z == 0;
                    printf("B.NE: Z=%d ", CURRENT_STATE.FLAG_Z);
                    break;
                case 0xB: // LT: N == 1
                    take_branch = CURRENT_STATE.FLAG_N == 1;
                    printf("B.LT: N=%d ", CURRENT_STATE.FLAG_N);
                    break;
                case 0xA: // GE: N == 0
                    take_branch = CURRENT_STATE.FLAG_N == 0;
                    printf("B.GE: N=%d ", CURRENT_STATE.FLAG_N);
                    break;
                case 0xD: // LE: Z == 1 || N == 1
                    take_branch = (CURRENT_STATE.FLAG_Z == 1 || CURRENT_STATE.FLAG_N == 1);
                    printf("B.LE: Z=%d N=%d ", CURRENT_STATE.FLAG_Z, CURRENT_STATE.FLAG_N);
                    break;
                case 0xC: // GT: Z == 0 && N == 0
                    take_branch = (CURRENT_STATE.FLAG_Z == 0 && CURRENT_STATE.FLAG_N == 0);
                    printf("B.GT: Z=%d N=%d ", CURRENT_STATE.FLAG_Z, CURRENT_STATE.FLAG_N);
                    break;
                default:
                    printf("Condición B.cond no reconocida: 0x%x\n", cond);
            }

            if (take_branch) {
                NEXT_STATE.PC = CURRENT_STATE.PC + inst.imm12;
                printf("tomada: salto a 0x%08lx\n", NEXT_STATE.PC);
                branch_taken = 1;
            } else {
                printf("no tomada: continúa\n");
            }
            break;
        }

        case OPCODE_CBZ:
            if (CURRENT_STATE.REGS[inst.rd] == 0) {
                NEXT_STATE.PC = CURRENT_STATE.PC + inst.imm12;
                printf("Ejecutando CBZ: X%d es 0, saltando a PC + %ld → 0x%08lx\n",
                       inst.rd, inst.imm12, NEXT_STATE.PC);
                branch_taken = 1;
            } else {
                printf("Ejecutando CBZ: X%d no es 0, continuando\n", inst.rd);
            }
            break;
        
        case OPCODE_CBNZ:
            if (!(CURRENT_STATE.REGS[inst.rd] == 0)) {
                NEXT_STATE.PC = CURRENT_STATE.PC + inst.imm12;
                printf("Ejecutando CBNZ: X%d no es 0, saltando a PC + %ld → 0x%08lx\n",
                       inst.rd, inst.imm12, NEXT_STATE.PC);
                branch_taken = 1;
            } else {
                printf("Ejecutando CBNZ: X%d es 0, continuando\n", inst.rd);
            }
            break;

        default:
            printf("Instrucción de salto no reconocida (Opcode: 0x%03x)\n", inst.opcode);
    }
    
    return branch_taken;
}

// Función para determinar el tipo de instrucción
int get_instruction_type(uint32_t opcode) {
    // Tipos de instrucciones
    #define TYPE_ARITHMETIC_LOGIC 0
    #define TYPE_MEMORY 1
    #define TYPE_BRANCH 2
    #define TYPE_UNKNOWN 3
    
    // Instrucciones aritméticas y lógicas
    if (opcode == OPCODE_ADD_IMM || opcode == OPCODE_ADD_EXT ||
        opcode == OPCODE_ADDS_IMM || opcode == OPCODE_ADDS_EXT ||
        opcode == OPCODE_SUBS_IMM || opcode == OPCODE_SUBS_EXT ||
        opcode == OPCODE_ANDS_REG || opcode == OPCODE_EOR_REG ||
        opcode == OPCODE_ORR_REG || opcode == INT_OPCODE_LSL ||
        opcode == INT_OPCODE_LSR || opcode == OPCODE_MOVZ ||
        opcode == OPCODE_MUL || opcode == OPCODE_HLT) {
        return TYPE_ARITHMETIC_LOGIC;
    }
    
    // Instrucciones de memoria
    if (opcode == OPCODE_LDUR || opcode == OPCODE_LDURH ||
        opcode == OPCODE_LDURB || opcode == OPCODE_STUR ||
        opcode == OPCODE_STURB || opcode == OPCODE_STURH) {
        return TYPE_MEMORY;
    }
    
    // Instrucciones de salto
    if (opcode == OPCODE_B || opcode == OPCODE_BR ||
        opcode == OPCODE_BL || opcode == OPCODE_B_COND ||
        opcode == OPCODE_CBZ || opcode == OPCODE_CBNZ) {
        return TYPE_BRANCH;
    }
    
    return TYPE_UNKNOWN;
}