#!/usr/bin/env bash
set -Eeuo pipefail

BIN=./ring
ERR=0
VALGRIND_AVAILABLE=$(command -v valgrind || echo "")

build() {
  make
  gcc -Wall -Wextra -std=c11 -o tester tester.c
}

run_valgrind_test() {
  local n=$1 c=$2 s=$3
  if [[ -n "$VALGRIND_AVAILABLE" ]]; then
    valgrind_output=$(valgrind --leak-check=full --error-exitcode=99 "$BIN" "$n" "$c" "$s" 2>&1)
    if [[ $? -eq 0 ]]; then
      printf "\t↳ ✅ valgrind: sin errores\n"
    else
      printf "\t↳ ❌ valgrind falló:\n"
      echo "$valgrind_output" | sed 's/^/\t    /'
      ERR=1
    fi
  else
    printf "\t↳ ⚠️ valgrind no está instalado\n"
  fi
}

test_basicos() {
  echo "✅ Test básicos de funcionamiento:"
  for case in \
    "3 0 0" \
    "5 10 2" \
    "4 -5 3" \
    "6 1000 5"
  do
    read n c s <<< "$case"
    out=$($BIN "$n" "$c" "$s" 2>&1)
    expected=$((c + n))
    if grep -q "Valor final recibido por el padre: $expected" <<< "$out"; then
      printf "  ✅ %s %2d %4d %2d → %d\n" "$BIN" "$n" "$c" "$s" "$expected"
      run_valgrind_test "$n" "$c" "$s"
    else
      printf "  ❌ %s %2d %4d %2d → esperado %d, salió:\n%s\n" "$BIN" "$n" "$c" "$s" "$expected" "$out"
      ERR=1
    fi
  done
}

test_invalidos() {
  echo
  echo "🚫 Casos inválidos:"
  local cmds=(
    "$BIN"
    "$BIN 2 0 0"
    "$BIN 3 0 3"
    "$BIN 4 0 -1"
    "$BIN a b c"
    "$BIN 3"
  )
  for cmd in "${cmds[@]}"; do
    if $cmd >/dev/null 2>&1; then
      printf "  ❌ '%s' debería fallar\n" "$cmd"
      ERR=1
    else
      printf "  ✅ '%s' falló como se espera\n" "$cmd"
    fi
  done
}

test_limites() {
  echo
  echo "🧪 Casos límite:"
  local limits=(
    "3 -100000 2"
    "3 2147483640 1"
    "1000 0 999"
    "3 0 2"
  )
  for case in "${limits[@]}"; do
    read n c s <<< "$case"
    out=$($BIN "$n" "$c" "$s" 2>&1)
    expected=$((c + n))
    if grep -q "Valor final recibido por el padre: $expected" <<< "$out"; then
      printf "  ✅ límite %2d %9d %3d → %d\n" "$n" "$c" "$s" "$expected"
    else
      printf "  ❌ límite %2d %9d %3d falló\n" "$n" "$c" "$s"
      ERR=1
    fi
  done
}

test_fallo_xwrite() {
  echo
  echo "💥 Pruebas extendidas de escritura (tester.c):"
  if ./tester; then
    echo "  ✅ Todas las pruebas de escritura pasaron correctamente"
  else
    echo "  ❌ Una o más pruebas de escritura fallaron (ver arriba)"
    ERR=1
  fi
}

build
test_basicos
test_invalidos
test_limites
test_fallo_xwrite

make clean
rm -f tester
exit $ERR
