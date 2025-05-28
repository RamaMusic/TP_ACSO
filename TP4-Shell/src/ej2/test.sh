#!/bin/bash

# ==== CONFIGURACIÓN ====
TEST_FILE="test.txt"
TEMP_OUT=$(mktemp)
VALGRIND_OUT=$(mktemp)
TOTAL=0
PASSED=0
FAILED=0
MEM_CLEAN=0
MEM_FAIL=0
VERBOSE=false

# ==== COLORES ====
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # Sin color

# ==== COMPILACIÓN ====
echo -e "${BLUE}⚙️  Compilando shell...${NC}"
make -s
if [ ! -f ./shell ]; then
    echo -e "${RED}❌ Error: el binario 'shell' no fue generado${NC}"
    exit 1
fi

# ==== ARCHIVO AUXILIAR ====
cat <<EOF > "$TEST_FILE"
imagen.png
documento.zip
imagen.jpg
EOF

# ==== FUNCIÓN DE TEST ====
run_test() {
    local input="$1"
    local expected="$2"
    local description="$3"
    ((TOTAL++))

    echo -e "${YELLOW}➤ Test $TOTAL: $description${NC}"
    echo -e "   ${BLUE}Comando:${NC} $input"
    echo -e "   ${BLUE}Esperado:${NC} $expected"

    ########## FUNCIONAL ##########
    output=$(echo -e "$input\nexit" | ./shell 2>&1)
    output=$(echo "$output" | sed '/^Shell started/d;/^Shell> *$/d;/Shell terminated/d')
    if echo "$output" | grep -q "$expected"; then
        echo -e "   ${BLUE}Salida: ${NC} $output"
        echo -e "   ${GREEN}✅ Funcionalidad PASÓ${NC}"
        ((PASSED++))
    else
        echo -e "   ${RED}❌ Funcionalidad FALLÓ${NC}"
        echo -e "   ${BLUE}Obtenido:${NC} $(echo "$output" | head -n 1)"
        ((FAILED++))
    fi

    ########## VALGRIND ##########
    echo -e "$input\nexit" | valgrind --leak-check=full --quiet --error-exitcode=42 ./shell > "$TEMP_OUT" 2> "$VALGRIND_OUT"
    status=$?
    if [ "$status" -eq 0 ]; then
        echo -e "   ${GREEN}✅ Memoria limpia (Valgrind)${NC}"
        ((MEM_CLEAN++))
    else
        echo -e "   ${RED}🧠 Leak detectado (Valgrind)${NC}"
        ((MEM_FAIL++))
        if [ "$VERBOSE" = true ]; then
            cat "$VALGRIND_OUT"
        fi
    fi
    echo ""
}

# ==== TESTS NORMALES Y EXTRA ====

run_test "echo hola" "hola" "Echo simple"
run_test "echo \"hola mundo\"" "hola mundo" "Echo con comillas dobles"
run_test "echo hola    mundo | wc -w" "2" "Espacios múltiples y pipe"
run_test "seq 10 | grep 5" "5" "Grep sobre secuencia"
run_test "seq 5 | tail -n 1" "5" "Tail de la última línea"
run_test "echo 'uno' 'dos' | wc -w" "2" "Dos palabras con comillas simples"
run_test "echo \"uno  dos\" | wc -m" "9" "Conteo de caracteres con espacios"
run_test "cat $TEST_FILE | grep .zip" "documento.zip" "Grep con patrón simple"
run_test "echo hola | grep hola | wc -l" "1" "Pipeline triple"
run_test "| echo hola" "Syntax error" "Pipe al inicio"
run_test "echo hola |" "Syntax error" "Pipe al final"
run_test "echo hola || wc" "Syntax error" "Pipe doble inválido"
run_test "inexistentecomando" "command not found" "Comando inválido"
run_test "   echo    prueba   " "prueba" "Comando con espacios en los extremos"
run_test "exit" "" "Comando exit"

# ==== CASOS BORDE EXTRA CORREGIDOS ====

run_test "ls | | wc" "Syntax error" "Pipe vacío entre comandos"
run_test $'echo\t\thola' "hola" "Tabulaciones entre comandos"
run_test "seq 100 | grep 5 | wc -l | cat | cat | cat" "19" "Pipeline largo con múltiples procesos"
run_test "echo $(seq -s ' ' 1 63)" "63" "Muchos argumentos (exacto MAX_ARGS)"
run_test "echo $(seq -s ' ' 1 64)" "Too many arguments" "Muchos argumentos (MAX_ARGS+1)"


# ==== RESUMEN FINAL ====

echo -e "${BLUE}============================================${NC}"
echo -e "         ${YELLOW}RESUMEN FINAL DE TESTS${NC}"
echo -e "   Total de tests:     $TOTAL"
echo -e "   ${GREEN}Funcionales OK:      $PASSED${NC}"
echo -e "   ${RED}Funcionales fallidos: $FAILED${NC}"
echo -e "   ${GREEN}Sin leaks de memoria: $MEM_CLEAN${NC}"
echo -e "   ${RED}Con leaks detectados: $MEM_FAIL${NC}"
echo -e "${BLUE}============================================${NC}"

# ==== LIMPIEZA ====
rm -f "$TEMP_OUT" "$VALGRIND_OUT" "$TEST_FILE"
make clean > /dev/null
