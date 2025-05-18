#!/bin/bash

# Compilar el shell
make

echo "🧪 Iniciando tests..."

# Crear archivo temporal de pruebas
TEMP_OUT=$(mktemp)

# Función para correr un test
run_test() {
    local input="$1"
    local expected="$2"
    local label="$3"

    echo -e "$input\nexit" | ./shell > "$TEMP_OUT" 2>&1
    if grep -q "$expected" "$TEMP_OUT"; then
        echo "✅ $label"
    else
        echo "❌ $label"
        echo "   Esperado: $expected"
        echo "   Salida:"
        cat "$TEMP_OUT"
        echo "-------------"
    fi
}

# Tests válidos
run_test "echo hola" "hola" "echo simple"
run_test "ls | wc -l" "" "pipeline básico (ls | wc -l)"
run_test "echo "'hola mundo' "hola mundo" "comillas dobles"
run_test "echo hola    mundo | wc -w" "2" "espacios múltiples y pipe"
run_test "whoami | grep $(whoami)" "$(whoami)" "grep usuario actual"
run_test "seq 10 | grep 5" "5" "grep número intermedio"
run_test "seq 5 | tail -n 1" "5" "última línea con tail"

# Casos borde
run_test "   echo    prueba   " "prueba" "comando con espacios iniciales y finales"
run_test "| echo hola" "" "pipe al inicio (debería fallar silenciosamente)"
run_test "echo hola |" "" "pipe al final (debería fallar silenciosamente)"
run_test "echo hola || wc" "" "doble pipe sin comando (debería fallar silenciosamente)"
run_test "inexistentecomando" "execvp" "comando inexistente"

# Cleanup
rm "$TEMP_OUT"
echo "✅ Todos los tests terminados"

make clean
