#!/bin/bash

# Colores para output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}===== PRUEBAS DE SHELL CON PIPES =====${NC}"

# Compilar el programa
echo -e "\n${YELLOW}Compilando shell.c...${NC}"
make
# gcc -o shell shell.c
# if [ $? -ne 0 ]; then
#     echo -e "${RED}Error al compilar${NC}"
#     exit 1
# fi
# echo -e "${GREEN}Compilación exitosa${NC}"

# # Crear archivos de prueba
# echo -e "\n${YELLOW}Creando archivos de prueba...${NC}"
# mkdir -p test_files
# cd test_files
# echo "Contenido de prueba 1" > archivo1.txt
# echo "Contenido de prueba 2" > archivo2.txt
# echo "Archivo ZIP simulado" > archivo.zip
# echo "Otro archivo ZIP" > otro.zip
# echo "Imagen PNG simulada" > imagen.png
# echo "Otra imagen PNG" > otra.png
# cd ..
# echo -e "${GREEN}Archivos de prueba creados${NC}"

# Función para ejecutar una prueba
run_test() {
    local test_name=$1
    local command=$2
    local expected_output=$3
    
    echo -e "\n${YELLOW}PRUEBA: $test_name${NC}"
    echo "Comando: $command"
    
    # Crear un archivo de entrada para el shell
    echo "$command" > test_input.txt
    echo "exit" >> test_input.txt
    
    # Ejecutar el shell con la entrada y capturar la salida
    actual_output=$(./shell < test_input.txt 2>&1 | grep -v "Shell>")
    
    # Verificar si la salida contiene el texto esperado
    if echo "$actual_output" | grep -q "$expected_output"; then
        echo -e "${GREEN}ÉXITO: Salida contiene: $expected_output${NC}"
        return 0
    else
        echo -e "${RED}ERROR: Salida esperada no encontrada${NC}"
        echo "Salida actual:"
        echo "$actual_output"
        return 1
    fi
}

# Pruebas básicas
run_test "Comando simple ls" "ls test_files" "archivo1.txt"
run_test "Comando simple echo" "echo hola mundo" "hola mundo"

# Pruebas con un pipe
run_test "Pipe simple ls | wc" "ls test_files | wc -l" "6"
run_test "Pipe ls | grep" "ls test_files | grep .zip" "archivo.zip"

# Pruebas con múltiples pipes
run_test "Múltiples pipes" "ls test_files | grep .txt | wc -l" "2"

# Extra credit: pruebas con argumentos entre comillas
run_test "Argumentos con comillas" "ls test_files | grep \".zip\"" "archivo.zip"
run_test "Múltiples patrones entre comillas" "ls test_files | grep \".png .zip\"" "archivo.zip"

# Limpiar archivos temporales
# rm -f test_input.txt
# rm -rf test_files
make clean

echo -e "\n${YELLOW}===== PRUEBAS FINALIZADAS =====${NC}"