#!/bin/bash
set -u  # Solo activamos error por variables no definidas (no usamos `-e` para evitar abortos silenciosos)

# ==== CONFIGURACIÓN ====
BIN=./ring
VALGRIND_AVAILABLE=$(command -v valgrind || echo "")
VALGRIND_OUT=$(mktemp)
TOTAL=0
PASSED=0
FAILED=0
MEM_CLEAN=0
MEM_FAIL=0
VERBOSE=false
[[ "${1:-}" == "--verbose" ]] && VERBOSE=true

# ==== COLORES ====
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# ==== COMPILACIÓN ====
echo -e "${BLUE}⚙️  Compilando proyecto...${NC}"
make -s
if [ ! -f "$BIN" ]; then
    echo -e "${RED}❌ Error: no se generó el binario 'ring'${NC}"
    exit 1
fi
echo ""

# ==== FUNCIÓN DE TESTS ====

run_test() {
    local n=$1 c=$2 s=$3 expected=$4 desc="$5"
    ((TOTAL++))
    echo -e "${YELLOW}➤ Test $TOTAL: $desc${NC}"
    echo -e "   ${BLUE}Comando: $BIN $n $c $s${NC}"
    echo -e "   ${BLUE}Esperado: Valor final recibido por el padre: $expected${NC}"

    if output=$($BIN "$n" "$c" "$s" 2>&1); then
        if echo "$output" | grep -q "Valor final recibido por el padre: $expected"; then
            echo -e "   ${GREEN}✅ Funcionalidad PASÓ${NC}"
            ((PASSED++))
        else
            echo -e "   ${RED}❌ Funcionalidad FALLÓ${NC}"
            echo -e "   ${BLUE}Obtenido:$(echo \"$output\" | grep 'Valor final recibido' || echo ' N/A')${NC}"
            ((FAILED++))
        fi
    else
        echo -e "   ${RED}❌ Ejecución fallida${NC}"
        ((FAILED++))
    fi

    if [ "$n" -ge 500 ]; then
        echo -e "   ${YELLOW}⚠️  Saltando Valgrind por stress test (n=$n)${NC}"
    elif [[ -n "$VALGRIND_AVAILABLE" ]]; then
        valgrind --leak-check=full --error-exitcode=42 "$BIN" "$n" "$c" "$s" >/dev/null 2> "$VALGRIND_OUT"
        status=$?
        if [ "$status" -eq 0 ]; then
            echo -e "   ${GREEN}✅ Memoria limpia (Valgrind)${NC}"
            ((MEM_CLEAN++))
        else
            echo -e "   ${RED}🧠 Leak detectado (Valgrind)${NC}"
            ((MEM_FAIL++))
            [ "$VERBOSE" = true ] && cat "$VALGRIND_OUT"
        fi
    else
        echo -e "   ${YELLOW}⚠️  Valgrind no instalado${NC}"
    fi

    echo ""
}

run_invalid() {
    local cmd="$1" desc="$2"
    ((TOTAL++))
    echo -e "${YELLOW}➤ Test $TOTAL: $desc${NC}"
    echo -e "   ${BLUE}Comando: $cmd${NC}"
    if $cmd >/dev/null 2>&1; then
        echo -e "   ${RED}❌ Debería fallar pero funcionó${NC}"
        ((FAILED++))
    else
        echo -e "   ${GREEN}✅ Falló como se espera${NC}"
        ((PASSED++))
    fi
    echo ""
}

# ==== TESTS ====

echo -e "${YELLOW}🔹 Tests básicos de funcionamiento:${NC}"
run_test 3   0    0   3    "Anillo de 3 procesos, valor 0, start 0"
run_test 5  10    2  15   "Anillo de 5 procesos, valor 10, start 2"
run_test 4  -5    3  -1   "Anillo de 4 procesos, valor -5, start 3"
run_test 6 1000   5 1006  "Anillo de 6 procesos, valor 1000, start 5"

echo -e "${YELLOW}🔹 Casos inválidos:${NC}"
run_invalid "$BIN"                               "Sin argumentos"
run_invalid "$BIN 2 0 0"                         "Solo dos argumentos"
run_invalid "$BIN 3 0 3"                         "Start fuera de rango"
run_invalid "$BIN 4 0 -1"                        "Start negativo"
run_invalid "$BIN a b c"                         "Argumentos no numéricos"
run_invalid "$BIN 3"                             "Un único argumento"

echo -e "${YELLOW}🔹 Casos límite:${NC}"
run_test 3  -100000 2  -99997        "Límite: valor inicial muy negativo"
run_test 3  2147483640 1 2147483643 "Límite: valor inicial cercano a INT_MAX"
run_test 500 0      499 500         "Límite seguro: n=500, start=499"
run_test 3   0        2   3          "Límite: start=2 en n=3"

run_test 5  42  4  47  "Start desde el último proceso"
run_test 3  1   2  4   "Anillo mínimo: n=3, start=n-1"

echo -e "${YELLOW}🔹 Validación de límites de enteros:${NC}"
# INT_MAX exacto
run_test 10 2147483637 0 2147483647 "Límite exacto: c + n = INT_MAX"
# INT_MAX + 1 → debería fallar
run_invalid "$BIN 10 2147483638 0" "Desborde positivo: c + n > INT_MAX"
# INT_MIN exacto
run_test 10 -2147483638 0 -2147483628 "Límite exacto: c + n = INT_MIN + n"
# INT_MIN - 1 → debería fallar
run_invalid "$BIN 10 -2147483639 0" "Desborde negativo: c + n < INT_MIN"

((TOTAL++))
echo -e "${YELLOW}➤ Test $TOTAL: Detección de deadlock con timeout${NC}"
echo -e "   ${BLUE}Comando: timeout 2s $BIN 3 0 0${NC}"
if timeout 2s $BIN 3 0 0 >/dev/null 2>&1; then
    echo -e "   ${GREEN}✅ Terminó correctamente (sin deadlock)${NC}"
    ((PASSED++))
else
    echo -e "   ${RED}❌ Timeout o error detectado (posible deadlock)${NC}"
    ((FAILED++))
fi

echo -e "${YELLOW}🔹 Pruebas extendidas de escritura/lectura (tester.c):${NC}"
((TOTAL++))
gcc -Wall -Wextra -std=c11 -o tester tester.c
if [ ! -f ./tester ]; then
    echo -e "   ${RED}❌ Error: no se compiló tester.c${NC}"
    ((FAILED++))
else
    if [[ -n "$VALGRIND_AVAILABLE" ]]; then
        valgrind --leak-check=full --error-exitcode=42 ./tester > /dev/null 2> "$VALGRIND_OUT"
        status=$?
        if [ "$status" -eq 0 ]; then
            echo -e "   ${GREEN}✅ tester.c pasó correctamente (sin leaks)${NC}"
            ((PASSED++))
            ((MEM_CLEAN++))
        else
            echo -e "   ${RED}❌ tester.c falló o tiene memory leaks${NC}"
            ((FAILED++))
            ((MEM_FAIL++))
            [ "$VERBOSE" = true ] && cat "$VALGRIND_OUT"
        fi
    else
        if ./tester; then
            echo -e "   ${GREEN}✅ tester.c pasó correctamente (sin Valgrind)${NC}"
            ((PASSED++))
        else
            echo -e "   ${RED}❌ tester.c falló (sin Valgrind)${NC}"
            ((FAILED++))
        fi
    fi
fi

echo -e "${YELLOW}🔹 Stress test sin Valgrind (n=1000):${NC}"
run_test 1000 0 999 1000 "Stress test: anillo de 1000 procesos"

# ==== RESUMEN FINAL ====
echo -e "${BLUE}============================================${NC}"
echo -e "         ${YELLOW}RESUMEN FINAL DE TESTS${NC}"
echo -e "   Total tests:         $TOTAL"
echo -e "   ${GREEN}Pasaron:              $PASSED${NC}"
echo -e "   ${RED}Fallaron:             $FAILED${NC}"
echo -e "   ${GREEN}Sin leaks Valgrind:   $MEM_CLEAN${NC}"
echo -e "   ${RED}Leaks detectados:      $MEM_FAIL${NC}"
echo -e "${BLUE}============================================${NC}"

# ==== LIMPIEZA ====
make clean >/dev/null
rm -f "$VALGRIND_OUT" tester test.txt

exit $((FAILED > 0 ? 1 : 0))
