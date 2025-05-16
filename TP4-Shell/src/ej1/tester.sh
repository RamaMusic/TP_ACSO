#!/usr/bin/env bash
set -Eeuo pipefail

make

BIN=./ring
ERR=0
VALGRIND_AVAILABLE=$(command -v valgrind || echo "")

run_valgrind_test() {
  local n=$1
  local c=$2
  local s=$3
  if [[ -n "$VALGRIND_AVAILABLE" ]]; then
    valgrind_output=$(valgrind --leak-check=full --error-exitcode=99 "$BIN" "$n" "$c" "$s" 2>&1)
    if [[ $? -eq 0 ]]; then
      printf "\t↳ ✅ valgrind: sin errores de memoria o descriptores\n"
    else
      printf "\t↳ ❌ valgrind detectó problemas:\n"
      echo "$valgrind_output" | sed 's/^/\t    /'
      ERR=1
    fi
  else
    printf "\t↳ ⚠️ valgrind no está instalado, omitiendo chequeo\n"
  fi
}

echo "Probando casos válidos:"
for n in 3 4 5 10; do
  for c in -5 0 1 42; do
    for s in $(seq 0 $((n - 1))); do
      out=$($BIN "$n" "$c" "$s" 2>&1)
      expected=$((c + n))
      if grep -q "Valor final recibido por el padre: $expected" <<< "$out"; then
        printf "  ✅ %s %2d %3d %2d → %d\n" "$BIN" "$n" "$c" "$s" "$expected"
        run_valgrind_test "$n" "$c" "$s"
      else
        printf "  ❌ %s %2d %3d %2d → esperado %d, salió:\n%s\n" \
               "$BIN" "$n" "$c" "$s" "$expected" "$out"
        ERR=1
      fi
    done
  done
done

echo
echo "Probando casos inválidos:"
invalid_cmds=(
  "$BIN"
  "$BIN 2 0 0"
  "$BIN 3 0 3"
  "$BIN 3 0 -1"
  "$BIN a b c"
  "$BIN 5"
  "$BIN 4 5"
  "$BIN 4 0 4"
  "$BIN 3 0 100"
  "$BIN 3 0 -999"
)
for cmd in "${invalid_cmds[@]}"; do
  if $cmd >/dev/null 2>&1; then
    printf "  ❌ '%s' debería fallar pero no lo hizo\n" "$cmd"
    ERR=1
  else
    printf "  ✅ '%s' falló como se espera\n" "$cmd"
  fi
done

echo
echo "Probando stress:"
if command -v timeout >/dev/null; then
  for n in 100 500 1000; do
    s=$((n / 2))
    c=$((RANDOM % 100))
    if timeout 5 $BIN "$n" "$c" "$s" >/dev/null 2>&1; then
      printf "  ✅ n=%d s=%d c=%d no se colgó\n" "$n" "$s" "$c"
    else
      printf "  ❌ n=%d s=%d c=%d se colgó o falló\n" "$n" "$s" "$c"
      ERR=1
    fi
  done
else
  echo "  ⚠️ 'timeout' no disponible; se omiten tests de stress"
fi

make clean
exit $ERR
