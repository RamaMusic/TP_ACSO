# Diario Técnico - Resolución del Trabajo Práctico "Bomba Binaria"

Este documento describe detalladamente el proceso seguido para resolver las fases propuestas en el trabajo práctico denominado "Bomba Binaria". El enfoque empleado consistió en el análisis minucioso del código ensamblador, uso intensivo de herramientas de depuración como GDB, inspección directa de memoria y desarrollo de scripts auxiliares en Python para facilitar la resolución de cada etapa planteada por la bomba.

---

## Fase 1: Comparación de Cadenas

### Análisis del código ensamblador

En esta primera fase, el objetivo era encontrar una cadena exacta requerida por la bomba. La función en ensamblador (`phase_1`) realizaba una comparación del input del usuario con una cadena almacenada en memoria:

```asm
401def: lea rsi, [rip+0xc7c62]     ; dirección del string oculto
401df6: call 4022b9 <strings_not_equal>
401dfb: test eax, eax
401dfd: jne 401e04 <explode_bomb>
```

### Resolución usando GDB

Para descubrir el string esperado se siguieron estos pasos:
- Se insertaron breakpoints en `explode_bomb` y en la línea posterior a la comparación.
- Se calculó la dirección del string:

```
0x401df6 + 0xc7c62 = 0x4c9a58
```

- Se examinó esta dirección en memoria mediante:

```
x/s 0x4c9a58
```

Este procedimiento reveló la cadena requerida:

```
"Confía en el tiempo, que suele dar dulces salidas a muchas amargas dificultades"
```

Al ingresar esta cadena en la bomba, la fase 1 fue superada exitosamente.

---

## Fase 2: Validación Numérica y Operaciones Binarias

### Análisis del código ensamblador

La fase 2 requería dos números enteros que cumplían dos condiciones específicas analizadas en la función auxiliar `misterio`:

- El número resultante de la operación `a = x + y - 32` debía contener exactamente 11 bits activos (bits en 1).
- El resultado de la operación XOR (`x ^ y`) debía ser negativo.

### Resolución mediante script en Python

Se desarrolló un script en Python para identificar pares válidos:

```python
for x in range(-10000, 10000):
    for y in range(-10000, 10000):
        a = x + y - 32
        if (bin(a).count('1') == 11) and ((x ^ y) < 0):
            print(x, y)
```

Una solución válida identificada fue:

```
-7921 10000
```

Introducir estos valores permitió desactivar la fase 2 con éxito.

---

## Fase 3: Recursividad y Búsqueda Binaria

### Análisis del problema

La fase 3 implicaba cargar un archivo externo llamado `palabras.txt`, que contenía un arreglo ordenado de palabras. La función `cuenta` realizaba una búsqueda binaria recursiva que retornaba un número basado en la suma del valor ASCII del primer carácter de cada palabra visitada durante la búsqueda.

### Script en Python para simulación

Se desarrolló un script que imitaba exactamente la búsqueda binaria realizada por el programa:

```python
for each word in palabras.txt:
    result = recursive_binary_search(word)
    if 401 <= result <= 799:
        store (word, result)
```

### Problemas encontrados

Inicialmente se produjo un error en la interpretación del formato requerido por el input:
- Se asumió incorrectamente que el formato era `<number> <string>`.
- Al inspeccionar el código ensamblador mediante GDB:

```
x/s 0x4c7099
→ "%s %d"
```

se reveló que el formato correcto era primero el string y luego el número.

Al corregir el formato del input, se ingresó la solución válida:

```
abatatar 782
```

Esto permitió superar con éxito la fase 3.

---

## Fase 4: Indexación Binaria y Operaciones Bitwise

### Análisis del problema

La cuarta fase requería un string de exactamente seis caracteres. La bomba realizaba lo siguiente:

- Extraía los 4 bits menos significativos (más bajos) de cada carácter del input.
- Sumaba los valores indexados en un arreglo denominado `array.0`, utilizando estos bits extraídos como índices.
- La suma total debía ser exactamente 56.

Para conocer el contenido del arreglo `array.0` se utilizó GDB:

```
array = [2, 13, 7, 14, 5, 10, 6, 15, 1, 12, 3, 4, 11, 8, 16, 9]
```

### Script en Python para búsqueda de soluciones

Se implementó un script en Python para probar todas las combinaciones posibles de índices hasta encontrar cadenas válidas:

```python
array = [2, 13, 7, 14, 5, 10, 6, 15, 1, 12, 3, 4, 11, 8, 16, 9]

for combination in all_possible_combinations(0-15, length=6):
    total = sum(array[index] for index in combination)
    if total == 56:
        generate_valid_string_from_combination(combination)
```

Esto generó cadenas válidas, una de las cuales fue:

```
001111
```

Al ingresar esta cadena en el programa, se logró desactivar satisfactoriamente la fase 4.

---

## Conclusión General

La resolución integral del trabajo práctico "Bomba Binaria" requirió aplicar técnicas avanzadas de ingeniería inversa, análisis profundo del código en ensamblador, depuración meticulosa con GDB y programación de scripts auxiliares en Python para simular y validar las soluciones. Cada fase presentó desafíos técnicos únicos y problemas específicos que fueron resueltos mediante un análisis meticuloso y un enfoque sistemático, demostrando el manejo práctico y efectivo de diversas herramientas y metodologías relacionadas con el análisis y la resolución de problemas complejos en contextos técnicos.