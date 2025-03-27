#include <stdio.h>
#include <stdint.h>
#include "shell.h"

// Declaraciones explícitas si no están en shell.h
extern CPU_State CURRENT_STATE, NEXT_STATE;
extern uint32_t mem_read_32(uint64_t address);

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
#define OPCODE_ADDS_IMM   0x588  // ADDS Xd, Xn, #imm
#define OPCODE_ADDS_EXT   0x558  // ADD Xd, Xn, Xm
#define OPCODE_SUBS_IMM   0x788  // SUBS Xd, Xn, #imm
#define OPCODE_SUBS_EXT   0x758  // SUBS Xd, Xn, Xm (sin inmediato)
#define OPCODE_ANDS_REG   0x750  // ANDS Xd, Xn, Xm (Shifted Register)
#define OPCODE_EOR_REG    0x650  // EOR Xd, Xn, Xm (Shifted Register)
#define OPCODE_HLT        0x6A2  // HLT
#define OPCODE_ORR_REG    0x550  // ORR Xd, Xn, Xm (Shifted Register)
#define OPCODE_B 0x05            // B bits 31–26
#define OPCODE_BR 0x6B0  // bits 31–21 para instrucción BR
#define OPCODE_BL 0x25  // bits 31–26
#define OPCODE_B_COND 0x54  // bits 31–24
#define OPCODE_LSR_IMM 0x69A  // opcode 31–21 según ensamblado real
#define OPCODE_UBFM_ALIAS 0x4B5   // bits 31–21 para UBFM (posible alias de LSL)
#define OPCODE_LSL_IMM    0xFFF   // Valor especial que vamos a usar internamente
#define OPCODE_LDUR 0x7C2  // bits 31–21 para LDUR X (verificado desde el binario real)
#define OPCODE_LDURH 0x3C2
#define OPCODE_LDURB 0x1C2
#define OPCODE_STUR 0x7C0  // bits 31–21
#define OPCODE_STURB 0x1C0  // bits 31–21
#define OPCODE_MOVZ 0x694  // bits 31–21 para MOVZ Xd, #imm, LSL #0 (hw = 0)

// Función para decodificar una instrucción
Instruction decode_instruction(uint32_t instruction) {
    Instruction inst;
    inst.opcode = (instruction >> 21) & 0x7FF;  // Extraer bits 31-21
    inst.opcode_31_26 = (instruction >> 26);
    inst.opcode_31_24 = (instruction >> 24);
    inst.rd = (instruction >> 0) & 0x1F;        // Extraer bits 4-0 (Registro destino)
    inst.rn = (instruction >> 5) & 0x1F;        // Extraer bits 9-5 (Registro fuente 1)
    inst.rm = (instruction >> 16) & 0x1F;       // Extraer bits 20-16 (Registro fuente 2, solo en EXT y REG)
    inst.shift = (instruction >> 22) & 0x3;     // Extraer bits 23-22 (Shift en IMM)
    inst.imm12 = 0;
    
    if (inst.opcode == OPCODE_ADDS_IMM || inst.opcode == OPCODE_SUBS_IMM) {
        inst.imm12 = (instruction >> 10) & 0xFFF; // Bits 21-10
    
        switch (inst.shift) {
            case 0b00: // No shift
                break;
            case 0b01: // LSL #12
                inst.imm12 <<= 12;
                break;
            default: // 0b10 y 0b11 son inválidos para esta instrucción
                printf("Shift inválido en instrucción ADDS/SUBS (IMM): %d\n", inst.shift);
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
    
    if (inst.opcode == OPCODE_MOVZ) {
        inst.rd = instruction & 0x1F;                // bits 4–0
        inst.imm12 = (instruction >> 5) & 0xFFFF;    // bits 20–5 → imm16
        uint8_t hw = (instruction >> 21) & 0x3;      // bits 22–21
        if (hw != 0) {
            printf("MOVZ solo implementado con hw = 0\n");
        }
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
        

        case OPCODE_HLT:
            printf("Deteniendo la simulación (HLT)\n");
            RUN_BIT = 0;
            break;;

        case OPCODE_B:
            NEXT_STATE.PC = CURRENT_STATE.PC + inst.imm12;
            printf("Ejecutando B: salto a PC + %ld → 0x%08lx\n", inst.imm12, NEXT_STATE.PC);
            CURRENT_STATE = NEXT_STATE;
            return; // Evitar el PC += 4 automático al final

        case OPCODE_BR:
            NEXT_STATE.PC = CURRENT_STATE.REGS[inst.rn];
            printf("Ejecutando BR: salto a dirección contenida en X%d → 0x%08lx\n",
                   inst.rn, NEXT_STATE.PC);
            CURRENT_STATE = NEXT_STATE;
            return;

        case OPCODE_BL:
            NEXT_STATE.REGS[30] = CURRENT_STATE.PC + 4; // Guardar dirección de retorno
            NEXT_STATE.PC = CURRENT_STATE.PC + inst.imm12; // Saltar
            printf("Ejecutando BL: salto a PC + %ld → 0x%08lx | X30 (LR) = 0x%08lx\n",
                   inst.imm12, NEXT_STATE.PC, NEXT_STATE.REGS[30]);
            CURRENT_STATE = NEXT_STATE;
            return;
        
        case OPCODE_B_COND: {
            int cond = inst.rd;
            int take_branch = 0;
        
            switch (cond) {
                case 0x0: // EQ: Z == 1
                    take_branch = (CURRENT_STATE.FLAG_Z == 1);
                    break;
                case 0x1: // NE: Z == 0
                    take_branch = (CURRENT_STATE.FLAG_Z == 0);
                    break;
                case 0xC: // LT: N != V (asumimos V = 0)
                    take_branch = (CURRENT_STATE.FLAG_N == 1);
                    break;
                case 0xD: // GE: N == V (asumimos V = 0)
                    take_branch = (CURRENT_STATE.FLAG_N == 0);
                    break;
                case 0xE: // LE: Z == 1 || N == 1
                    take_branch = (CURRENT_STATE.FLAG_Z == 1 || CURRENT_STATE.FLAG_N == 1);
                    break;
                case 0xF: // GT: Z == 0 && N == 0
                    take_branch = (CURRENT_STATE.FLAG_Z == 0 && CURRENT_STATE.FLAG_N == 0);
                    break;
                default:
                    printf("Condición B.cond no reconocida: 0x%x\n", cond);
            }
        
            if (take_branch) {
                NEXT_STATE.PC = CURRENT_STATE.PC + inst.imm12;
                printf("B.cond tomada (cond=0x%x): salto a 0x%08lx\n", cond, NEXT_STATE.PC);
                CURRENT_STATE = NEXT_STATE;
                return;
            } else {
                printf("B.cond no tomada (cond=0x%x): continúa\n", cond);
            }
        
            break;
        }

        case OPCODE_LSL_IMM:
        NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] << inst.imm12;
        printf("Ejecutando LSL (IMM): X%d = X%d << %ld\n",
               inst.rd, inst.rn, inst.imm12);
        break;

        case OPCODE_LSR_IMM:
            NEXT_STATE.REGS[inst.rd] = CURRENT_STATE.REGS[inst.rn] >> inst.imm12;
            printf("Ejecutando LSR (IMM): X%d = X%d >> %ld → 0x%lx\n",
                inst.rd, inst.rn, inst.imm12, NEXT_STATE.REGS[inst.rd]);
            break;

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

        case OPCODE_MOVZ:
            NEXT_STATE.REGS[inst.rd] = inst.imm12 & 0xFFFF;
            printf("Ejecutando MOVZ: X%d = 0x%lx\n", inst.rd, NEXT_STATE.REGS[inst.rd]);
            break;

        
        
        default:
            printf("Instrucción no reconocida (Opcode: 0x%03x)\n", inst.opcode);
    }
    
    // 5️⃣ Avanzar el PC a la siguiente instrucción
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
    
    // 6️⃣ Actualizar el estado del CPU
    CURRENT_STATE = NEXT_STATE;
}
