#!/bin/bash

set -e

# Configuración
VERBOSE=false  # Cambiar a true para ver detalles

# Compilar el shell
make

echo "🧪 Iniciando tests..."

# Archivos temporales
TEMP_OUT=$(mktemp)
TEST_FILE="test.txt"

# Crear archivo de prueba
cat <<EOF > "$TEST_FILE"
imagen.png
documento.zip
imagen.jpg
EOF

# Función para correr un test
run_test() {
    local input="$1"
    local expected="$2"
    local label="$3"

    echo -e "$input\nexit" | ./shell > "$TEMP_OUT" 2>&1
    if grep -q "$expected" "$TEMP_OUT"; then
        echo "✅ $label"
        if [ "$VERBOSE" = true ]; then
            echo "   Comando: $input"
            echo "   Esperado: $expected"
            echo "   Salida:"
            cat "$TEMP_OUT"
            echo "-------------"
        fi
    else
        echo "❌ $label"
        echo "   Comando: $input"
        echo "   Esperado: $expected"
        echo "   Salida:"
        cat "$TEMP_OUT"
        echo "-------------"
    fi
}

# Tests válidos
run_test "echo hola" "hola" "echo simple"
run_test "ls | wc -l" "" "pipeline básico (ls | wc -l)"
run_test "echo \"hola mundo\"" "hola mundo" "comillas dobles"
run_test "echo hola    mundo | wc -w" "2" "espacios múltiples y pipe"
run_test "whoami | grep $(whoami)" "$(whoami)" "grep usuario actual"
run_test "seq 10 | grep 5" "5" "grep número intermedio"
run_test "seq 5 | tail -n 1" "5" "última línea con tail"
run_test "echo 'uno' 'dos' | wc -w" "2" "dos strings entre comillas simples"
run_test "echo \"uno  dos\" | wc -m" "9" "espacios internos en comillas dobles"
run_test "cat $TEST_FILE | grep \".png .zip\"" "" "grep con patrón entre comillas (extra)"
run_test "echo hola | grep hola | wc -l" "1" "pipeline triple"

# Casos borde
run_test "   echo    prueba   " "prueba" "comando con espacios iniciales y finales"
run_test "| echo hola" "" "pipe al inicio (debería fallar silenciosamente)"
run_test "echo hola |" "" "pipe al final (debería fallar silenciosamente)"
run_test "echo hola || wc" "" "doble pipe sin comando (debería fallar silenciosamente)"
run_test "inexistentecomando" "inexistentecomando: command not found" "comando inexistente"
run_test "" "" "línea vacía"
run_test "     " "" "línea con solo espacios"
run_test "exit" "" "comando de salida"

# Cleanup
rm "$TEMP_OUT"
rm "$TEST_FILE"
make clean

echo "✅ Todos los tests terminados"
