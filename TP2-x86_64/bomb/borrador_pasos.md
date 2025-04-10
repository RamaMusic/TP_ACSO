# Resolución Técnica – TP Bomba Binaria

A continuación se detallan los pasos realizados para resolver la Fase 1 y Fase 2 del trabajo práctico estilo "bomba binaria", incluyendo análisis de código ensamblador, técnicas utilizadas y resultados obtenidos.

---

## 🔹 Fase 1 – Comparación de Cadenas

### Análisis del ensamblador

La función `phase_1` toma como argumento un string (input del usuario) y lo compara contra una cadena constante embebida en el binario. A través del análisis del código ensamblador, se identificaron los siguientes pasos:

```asm
401def: lea rsi, [rip+0xc7c62]     ; carga un puntero a string constante
401df6: call 4022b9 <strings_not_equal>
401dfb: test eax, eax
401dfd: jne 401e04 <explode_bomb>
```

Esto indica que se compara el string ingresado (en `rdi`) con una cadena localizada en la dirección `rip + 0xc7c62`.

---

### Técnica utilizada para recuperar el string

1. Se colocó un breakpoint en `explode_bomb` para detener la ejecución en caso de fallo.
2. También se colocó un breakpoint en `401dfb`, justo después de la comparación con `strings_not_equal`.
3. En el debugger (`gdb`), se calculó la dirección del string esperado:

   ```
   0x401df6 + 0xc7c62 = 0x4c9a58
   ```

4. Se inspeccionó la memoria en esa dirección con:

   ```
   x/s 0x4c9a58
   ```

5. Esto reveló el string esperado:

   ```
   "Confía en el tiempo, que suele dar dulces salidas a muchas amargas dificultades"
   ```

---

### Resultado

Este string fue ingresado como input de la fase 1, y **la bomba no explotó**, por lo que la fase quedó **desactivada correctamente**.

---

## 🔹 Fase 2 – Análisis y Condiciones Lógicas

### Análisis del ensamblador

La función `phase_2` toma dos números enteros como input del usuario. A partir del análisis de código ensamblador, se identificó la siguiente lógica:

1. Se parsean dos enteros `x` e `y` del input usando `strtol`.
2. Se llama a la función `misterio` con:
   ```c
   a = x + y - 32
   b = x
   y → pasado como tercer parámetro (edx)
   ```

---

### Análisis de la función `misterio`

Dentro de `misterio(a, b)` se realizan dos verificaciones críticas:

1. **Conteo de bits en 1 en `a`**:  
   Se recorre bit a bit el valor `a` y se cuentan cuántos bits están en 1.  
   **Condición**: `a` debe tener exactamente **11 bits en 1**.

2. **Verificación de signo**:
   Se calcula `x ^ y` (XOR entre los dos números).  
   **Condición**: `x ^ y` debe ser **negativo**, es decir, su bit de signo (bit 31) debe estar activado.

Si **cualquiera** de estas dos condiciones no se cumple, la bomba explota.

---

### Estrategia de resolución

Para encontrar un par válido de `(x, y)`, se escribió un script que:

- Recorre valores posibles de `x` e `y` en un rango amplio (`-10.000` a `10.000`).
- Evalúa si `x + y - 32` tiene exactamente 11 bits en 1.
- Verifica que `x ^ y` tenga el bit de signo en 1 (sea negativo).

---

### Resultados

Se encontró que los siguientes pares cumplen ambas condiciones:

```
(x, y)
-------
(-7921, 10000)
(-7920,  9999)
(-7919,  9998)
(-7918,  9997)
(-7917,  9996)
(-7916,  9995)
(-7915,  9994)
(-7914,  9993)
(-7913,  9992)
(-7912,  9991)
```

En particular, se ingresó el par:

```
-7921 10000
```

Y **la fase 2 fue desactivada exitosamente** sin explotar la bomba.

---

## ✅ Conclusión

Ambas fases fueron resueltas utilizando análisis de código ensamblador, técnicas de debugging con `gdb`, inspección directa de memoria, y scripting para automatizar validaciones. Las condiciones lógicas

## 🔹 Fase 3 – Resolución Detallada

La fase 3 de la bomba binaria fue considerablemente más compleja que las anteriores, involucrando análisis de código ensamblador, simulación recursiva, parsing, manejo de memoria dinámica y lectura de un archivo externo.

---

### 🧠 Análisis general

La función `phase_3` realiza los siguientes pasos:

1. Reserva memoria dinámica con `malloc`.
2. Usa `sscanf(input, "%s %d", ...)` para extraer dos elementos del input del usuario:
   - Un **string** (en un buffer dinámico).
   - Un **número entero** (guardado en `[rsp]`).
3. Llama a la función `readlines`, que carga un arreglo de strings desde un archivo.
4. Llama a la función recursiva `cuenta(string, arreglo, 0, n-1)` y guarda el resultado.
5. Realiza tres validaciones críticas:
   - Que `sscanf` haya parseado exactamente 2 valores.
   - Que el resultado de `cuenta` esté en el rango `[401, 799]`.
   - Que el número ingresado por el usuario coincida **exactamente** con el valor retornado por `cuenta`.

---

### 📂 `readlines`: carga del archivo

Se descubrió (mediante inspección de memoria con GDB):

```gdb
x/s 0x4c708c
```

Que la función `readlines` abre el archivo:

```
palabras.txt
```

Este archivo contiene 10.784 palabras ordenadas alfabéticamente, cada una terminada en `\n` (reemplazado luego por `\0`).

---

### 🔄 `cuenta`: búsqueda binaria con suma de caracteres

La función `cuenta(s, arreglo, inicio, fin)` implementa:

- Una **búsqueda binaria recursiva** sobre el arreglo de palabras.
- En cada paso, compara `s` con la palabra del medio (`strcmp`).
- Si encuentra coincidencia exacta, retorna el valor ASCII del **primer carácter** de la palabra.
- Si no, se mueve a izquierda o derecha, y **suma el primer carácter del nodo actual** al resultado de la recursión.
- Si `s` no está en el arreglo, **la bomba explota**.

---

### 💡 Script auxiliar en Python

Se desarrolló un script en Python para simular la función `cuenta()` y buscar todas las palabras de `palabras.txt` que cumplieran con las condiciones de la fase.

#### 🧰 Lógica del script (pseudocódigo):

```
para cada palabra en palabras.txt:
    calcular cuenta(palabra, arreglo, 0, n-1)
    si resultado está entre 401 y 799:
        guardar (valor, palabra)
```

Este script permitió identificar combinaciones válidas para desactivar la bomba.

---

### ⚠️ Problema encontrado: orden incorrecto del input

Inicialmente se asumió que el input debía ser:

```
<number> <string>
```

Pero tras inspeccionar el formato de `sscanf` en el binario:

```gdb
x/s 0x4c7099
→ "%s %d"
```

Se descubrió que el orden correcto era **primero el string, luego el número**.

Este error causó que `sscanf` interpretara mal los valores, dejando `"782"` como string y provocando que la bomba explotara, incluso si la lógica era correcta.

---

### ✅ Resolución final

Se ingresó correctamente:

```
abatatar 782
```

Ya que:
- `"abatatar"` está en `palabras.txt`.
- `cuenta("abatatar", palabras, 0, n-1) = 782`.
- Y el número está en el rango válido.

Con eso, **la fase 3 fue desactivada exitosamente**.

---
## 🔹 Fase 4 – Resolución Detallada

La fase 4 presenta una verificación sobre un string de exactamente 6 caracteres. A diferencia de las fases anteriores, esta no requiere interacción con estructuras de datos externas ni recursion, pero sí involucra indexación en un arreglo y operaciones a nivel de bits.

---

### 🧠 Análisis del ensamblador

La función `phase_4` realiza los siguientes pasos:

1. **Verifica que el input tenga exactamente 6 caracteres**:
   ```asm
   call string_length
   cmp eax, 0x6
   jne explode_bomb
   ```
   → Cualquier input que no tenga longitud 6 explota la bomba.

2. **Inicializa punteros y variables**:
   - `rsi` apunta a un arreglo llamado `array.0`, ubicado en memoria (`0x4cde40`).
   - `ecx = 0` acumulará una suma.
   - `rdi` apunta al final del string (`input + 6`).

3. **Itera sobre cada carácter del string** (6 veces):
   ```asm
   edx = input[i] & 0x0F         ; extrae los 4 bits bajos del carácter
   ecx += array[edx]             ; suma el valor correspondiente
   ```
   Es decir, se usa `ord(c) & 0x0F` como índice en el arreglo `array.0`.

4. **Verifica que la suma total sea 56**:
   ```asm
   cmp ecx, 0x38    ; 0x38 = 56
   jne explode_bomb
   ```

---

### 🧮 Contenido de `array.0`

Se inspeccionó el contenido con GDB:

```gdb
x/16dw 0x4cde40
```

Y se obtuvo:

```python
array = [2, 13, 7, 14, 5, 10, 6, 15, 1, 12, 3, 4, 11, 8, 16, 9]
          # 0   1   2   3   4    5   6   7   8   9    A   B   C   D   E   F
```

---

### 💡 Resolución

La lógica final es:

- Se debe encontrar un string de **6 caracteres** donde la suma de `array[ord(c) & 0x0F]` sea **exactamente 56**.
- Los caracteres pueden ser cualquier byte, pero por simplicidad se usaron dígitos `'0'`–`'9'` y letras `'a'`–`'f'` (hexadecimal).

---

### 🧰 Script auxiliar en Python

Se utilizó un script para probar combinaciones posibles de 6 índices (`0–15`) que cumplieran la condición. El script probó todas las combinaciones posibles y las convirtió a strings legibles.

#### 🧠 Lógica del script (pseudocódigo):

```
array = [valores de array.0]

para cada combinación de 6 valores entre 0 y 15:
    si sum(array[i] for i en combinación) == 56:
        convertir cada índice i en un carácter ASCII con ord(c) & 0x0F == i
        guardar la string como válida
```

---

### ✅ Strings válidas encontradas

Las siguientes cadenas fueron encontradas como válidas:

```
001111
001139
00115e
00117c
001193
0011c7
0011e5
0012ee
001319
00133c
```

Cada una tiene exactamente 6 caracteres, y su suma indexada cumple la condición `== 56`.

---

### 🎯 Ejemplo de input correcto:

```
abatatar 782
001111
```

- `"abatatar 782"` fue el input usado para resolver la fase 3.
- `"001111"` es uno de los inputs válidos para fase 4.

---

### ✅ Resultado

Con uno de los strings válidos ingresado, **la fase 4 fue desactivada exitosamente**, sin que la bomba explotara.

---
