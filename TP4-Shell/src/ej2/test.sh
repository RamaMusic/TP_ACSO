#!/bin/bash

# CONFIGURACIÓN
TEST_FILE="test.txt"
TEMP_OUT=$(mktemp)
VALGRIND_OUT=$(mktemp)
TOTAL=0
PASSED=0
FAILED=0
MEM_CLEAN=0
MEM_FAIL=0
VERBOSE=false

# COLORES
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# COMPILACIÓN
echo -e "${BLUE}⚙️  Compilando shell...${NC}"
make -s
if [ ! -f ./shell ]; then
    echo -e "${RED}❌ Error: el binario 'shell' no fue generado${NC}"
    exit 1
fi

# ARCHIVO AUXILIAR
cat <<EOF > "$TEST_FILE"
imagen.png
documento.zip
imagen.jpg
EOF

# FUNCIÓN PRINCIPAL DE TEST
run_test() {
    local input="$1"
    local expected="$2"
    local description="$3"
    ((TOTAL++))

    echo -e "${YELLOW}➤ Test $TOTAL: $description${NC}"
    echo -e "   ${BLUE}Comando:${NC} $input"
    echo -e "   ${BLUE}Esperado:${NC} $expected"

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

# TESTS FUNCIONALES BÁSICOS
run_test "echo hola" "hola" "Echo simple"
run_test "echo \"hola mundo\"" "hola mundo" "Echo con comillas dobles"
run_test "echo hola    mundo | wc -w" "2" "Espacios múltiples y pipe"
run_test "seq 10 | grep 5" "5" "Grep sobre secuencia"
run_test "seq 5 | tail -n 1" "5" "Tail de la última línea"
run_test "echo 'uno' 'dos' | wc -w" "2" "Comillas simples como argumentos"
run_test "echo \"uno  dos\" | wc -m" "9" "Conteo de caracteres con espacios"
run_test "cat $TEST_FILE | grep .zip" "documento.zip" "Búsqueda de extensión"
run_test "echo hola | grep hola | wc -l" "1" "Pipeline triple"
run_test "/bin/echo hola" "hola" "Comando con ruta absoluta"
run_test "ls | sort | uniq" "" "Pipeline real sin wc (validez estructural)"

# ERRORES DE SINTAXIS
run_test "| echo hola" "Syntax error" "Pipe al inicio"
run_test "echo hola |" "Syntax error" "Pipe al final"
run_test "echo hola || wc" "Syntax error" "Pipe doble"
run_test "ls | | wc" "Syntax error" "Pipe vacío entre comandos"
run_test "| | | |" "Syntax error" "Múltiples pipes vacíos"
run_test "|||" "Syntax error" "Tres pipes consecutivos"
run_test "| | hola |" "Syntax error" "Comando entre pipes vacíos"

# ERRORES DE PARSING Y COMANDOS INVÁLIDOS
run_test "inexistentecomando" "command not found" "Comando inexistente"
run_test "echo \"hola" "hola" "Comillas abiertas sin cerrar"
run_test "   echo    prueba   " "prueba" "Espaciado irregular"
run_test $'echo\t\thola' "hola" "Tabulaciones entre palabras"

# COMANDOS ESPECIALES Y VALORES BORDES
run_test "exit" "" "Comando de salida"
run_test "exit | wc" "" "Exit dentro de pipeline"
run_test "yes | head -n 5" $'y\ny\ny\ny\ny' "Yes truncado por head"
run_test "echo \"\"" "" "Echo con string vacío"
run_test "echo hola | grep -v hola" "" "Grep que descarta salida"
run_test "cat /dev/null | wc -l" "0" "Conteo sobre input vacío"

# ARGUMENTOS EXTREMOS
run_test "echo $(seq -s ' ' 1 63)" "63" "Límite exacto de argumentos"
run_test "echo $(seq -s ' ' 1 64)" "Too many arguments" "Exceso de argumentos"

# EXTRA CREDIT: COMANDOS COMPLEJOS
run_test "cat $TEST_FILE | grep -E \".png\$|.zip\$\"" $'imagen.png\ndocumento.zip' "Extra Credit: grep con regex compuesta"

# STRESS TEST: PIPELINE LARGO (200 PROCESOS)
PIPE_CHAIN=$(printf 'grep . | %.0s' {1..199}; echo tail -n 1)
run_test "cat $TEST_FILE | $PIPE_CHAIN" "documento.zip" "Pipeline de 200 procesos con grep"

# RESUMEN FINAL
echo -e "${BLUE}============================================${NC}"
echo -e "         ${YELLOW}RESUMEN FINAL DE TESTS${NC}"
echo -e "   Total de tests:     $TOTAL"
echo -e "   ${GREEN}Funcionales OK:      $PASSED${NC}"
echo -e "   ${RED}Funcionales fallidos: $FAILED${NC}"
echo -e "   ${GREEN}Sin leaks de memoria: $MEM_CLEAN${NC}"
echo -e "   ${RED}Con leaks detectados: $MEM_FAIL${NC}"
echo -e "${BLUE}============================================${NC}"

# LIMPIEZA
rm -f "$TEMP_OUT" "$VALGRIND_OUT" "$TEST_FILE"
make clean > /dev/null
