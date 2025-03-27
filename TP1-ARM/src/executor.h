#ifndef _EXECUTOR_H_
#define _EXECUTOR_H_

#include "instruction.h"

// Función para ejecutar instrucciones aritméticas y lógicas
void execute_arithmetic_logic(Instruction inst);

// Función para ejecutar instrucciones de memoria
void execute_memory(Instruction inst);

// Función para ejecutar instrucciones de salto
// Retorna 1 si se tomó el salto, 0 en caso contrario
int execute_branch(Instruction inst);

// Función para determinar el tipo de instrucción
// Retorna: 0 = aritmética/lógica, 1 = memoria, 2 = salto, 3 = desconocida
int get_instruction_type(uint32_t opcode);

#endif // _EXECUTOR_H_