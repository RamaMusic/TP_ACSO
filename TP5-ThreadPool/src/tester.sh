#!/usr/bin/env bash
#
# tester.sh — Ejecución completa de build, tests, Valgrind memcheck y Helgrind

# Archivos de log temporales
VALGRIND_MEMCHECK_LOG="valgrind_memcheck.log"
VALGRIND_HELGRIND_LOG="valgrind_helgrind.log"
TEST_TARGET="custom"

# 🔄 Limpieza inicial
echo "🧹  Limpieza inicial: eliminando compilados y logs antiguos..."
make clean > /dev/null 2>&1
rm -f $VALGRIND_MEMCHECK_LOG $VALGRIND_HELGRIND_LOG

# ⚙️ Función auxiliar para abortar en errores y limpiar salvo el log que falló
abort_with_cleanup() {
  local failed_log=$1
  echo "⚠️  Finalizando con errores. Conservando log: $failed_log"
  # Borra el otro log si existe
  [[ "$failed_log" != "$VALGRIND_MEMCHECK_LOG" ]] && rm -f "$VALGRIND_MEMCHECK_LOG"
  [[ "$failed_log" != "$VALGRIND_HELGRIND_LOG" ]] && rm -f "$VALGRIND_HELGRIND_LOG"
  make clean > /dev/null 2>&1
  exit 1
}

# 🧹 Paso 1
echo "🧹  Step 1/6: Limpiando build anterior..."
make clean
[[ $? -ne 0 ]] && abort_with_cleanup ""

# 🏗️ Paso 2
echo
echo "🏗️  Step 2/6: Compilando (make custom)..."
make $TEST_TARGET
[[ $? -ne 0 ]] && abort_with_cleanup ""

# ▶️ Paso 3
echo
echo "▶️  Step 3/6: Ejecutando tests funcionales (./threadpool --all)..."
./threadpool --all
[[ $? -ne 0 ]] && abort_with_cleanup ""

# 🔍 Paso 4
echo
echo "🔍  Step 4/6: Ejecutando Valgrind memcheck..."
valgrind --tool=memcheck --leak-check=full \
         ./threadpool --all &> "$VALGRIND_MEMCHECK_LOG"

# Buscamos errores reales (Invalid read/write, definitely lost, etc)
critical_errors=$(grep -E '==.*ERROR SUMMARY: [1-9]' "$VALGRIND_MEMCHECK_LOG")
definitely_lost=$(grep -E 'definitely lost: [1-9][0-9]* bytes' "$VALGRIND_MEMCHECK_LOG")

if [[ -n "$critical_errors" && -n "$definitely_lost" ]]; then
  echo "❌  Valgrind memcheck detectó errores críticos (ver $VALGRIND_MEMCHECK_LOG)"
  abort_with_cleanup "$VALGRIND_MEMCHECK_LOG"
else
  echo "✅  Valgrind memcheck: OK (no hay pérdidas definitivas ni errores críticos)"
fi


# 🐞 Paso 5
echo
echo "🐞  Step 5/6: Ejecutando Valgrind Helgrind (race detection)..."
valgrind --tool=helgrind --error-exitcode=1 \
         ./threadpool --all &> "$VALGRIND_HELGRIND_LOG"
if [[ $? -ne 0 ]]; then
  echo "❌  Helgrind detectó data races (ver $VALGRIND_HELGRIND_LOG)"
  abort_with_cleanup "$VALGRIND_HELGRIND_LOG"
else
  echo "✅  Valgrind Helgrind: OK"
fi

# 🧹 Paso 6
echo
echo "🧹  Step 6/6: Limpieza final completa..."
make clean
rm -f "$VALGRIND_MEMCHECK_LOG" "$VALGRIND_HELGRIND_LOG"
echo "✅  Todos los archivos temporales eliminados"

# 🎉 Éxito total
echo
echo "🎉  ¡Todos los pasos completados correctamente!"
