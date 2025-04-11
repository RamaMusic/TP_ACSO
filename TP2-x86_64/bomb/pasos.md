# Resolución Técnica – TP Bomba Binaria

Este documento presenta una descripción detallada y paso a paso del proceso seguido para resolver las diferentes fases del trabajo práctico denominado "Bomba Binaria". El enfoque se basó en analizar cuidadosamente el código ensamblador, utilizar el debugger GDB para inspeccionar la memoria y realizar scripts en Python que automatizaran tareas específicas y complejas.

---

## Fase 1 – Comparación de Cadenas

### Análisis del código ensamblador

Al inicio, revisé la función `phase_1` y observé que la misma comparaba la cadena que yo ingresaba con otra almacenada dentro del binario. El código ensamblador indicaba claramente una comparación directa:

```asm
401def: lea rsi, [rip+0xc7c62]     ; carga puntero a string constante
401df6: call 4022b9 <strings_not_equal>
401dfb: test eax, eax
401dfd: jne 401e04 <explode_bomb>
```

### Resolución utilizando GDB

Para obtener el valor correcto:

- Coloqué un breakpoint en `explode_bomb` y otro justo después de la comparación.
- Calculé la dirección del string sumando `rip` y el offset dado.
- Inspeccioné esta dirección con el comando de GDB:

```
x/s 0x4c9a58
```

Encontré así la cadena requerida:

```
"Confía en el tiempo, que suele dar dulces salidas a muchas amargas dificultades"
```

Ingresar este valor resolvió exitosamente la primera fase.

---

## Fase 2 – Validación Numérica y Condiciones Binarias

### Análisis inicial del ensamblador

La segunda fase requería dos números enteros. Al revisar el código, noté que había condiciones complejas en la función `misterio`. Puntualmente, se validaba que el resultado de `x + y - 32` tuviera exactamente 11 bits activos y que el resultado del XOR (`x ^ y`) fuera negativo.

### Desarrollo de un script en Python

Decidí crear un script en Python para resolver esto de forma más rápida:

```python
for x in range(-10000, 10000):
    for y in range(-10000, 10000):
        a = x + y - 32
        if (bin(a).count('1') == 11) and ((x ^ y) < 0):
            print(x, y)
```

Este script encontró rápidamente varios pares válidos. Entre estos, usé el par `-7921 10000` y con eso pude avanzar a la siguiente fase sin problema.

---

## Fase 3 – Búsqueda Binaria y Recursión

### Entendiendo la fase

La fase 3 fue algo más complicada. Observé en el código que el programa abría un archivo llamado `palabras.txt`. La función `cuenta` realizaba una búsqueda binaria sobre las palabras cargadas desde este archivo, acumulando valores ASCII según los nodos que recorría.

### Script en Python para simulación

Para resolverlo más rápido y evitar hacerlo manualmente, escribí otro script que simulaba exactamente esta lógica:

```python
for word in palabras:
    resultado = simular_cuenta(word)
    if 401 <= resultado <= 799:
        guardar (word, resultado)
```

### Problema encontrado y solución

Tuve un problema porque inicialmente asumí que el input debía ser primero el número y luego el string, pero al analizar con GDB usando:

```
x/s 0x4c7099
→ "%s %d"
```

descubrí que el formato era el contrario (primero el string, luego el número). Tras corregir este detalle, ingresé el input correcto:

```
abatatar 782
```

Esto resolvió satisfactoriamente la fase 3.

---

## Fase 4 – Indexación Binaria y Operaciones a nivel de bits

### Análisis del problema

Esta fase esperaba un string de exactamente 6 caracteres. El programa extraía los 4 bits más bajos de cada carácter y utilizaba esos valores para indexar un arreglo llamado `array.0`. La condición era que la suma resultante fuera exactamente 56.

Utilizando GDB inspeccioné el contenido del arreglo:

```
array = [2, 13, 7, 14, 5, 10, 6, 15, 1, 12, 3, 4, 11, 8, 16, 9]
```

### Uso de un script en Python

Escribí otro script en Python para probar todas las combinaciones posibles de índices que cumplieran la condición:

```python
for c in product(range(16), repeat=6):
    if sum(array[i] for i in c) == 56:
        guardar_cadena(c)
```

Entre varias cadenas válidas, escogí:

```
001111
```

Esto me permitió avanzar sin problemas.

---

## Fase Secreta – Descubrimiento y resolución

### Encontrando la fase secreta

Luego de resolver la fase 4, decidí continuar revisando el código con GDB y me encontré con una función `phase_defused` que esperaba un input adicional especial después de completar las cuatro fases.

En una inspección detallada, descubrí que uno de esos inputs debía ser exactamente el string `abrete_sesamo`. Al introducir este string adicional junto con dos enteros previos, activé la fase secreta (`secret_phase`).

### Analizando y resolviendo la fase secreta

La función secreta solicitaba otro número entre 1 y 1000, que era usado en una búsqueda binaria recursiva llamada `fun7`. Al analizarla en detalle, encontré que esta función codificaba el camino dentro de un árbol binario:

- Hacia la izquierda: multiplicaba por 2.
- Hacia la derecha: multiplicaba por 2 y sumaba 1.

Para resolver la fase, el resultado debía ser exactamente 2, lo que significaba un camino de primero a la izquierda y luego a la derecha.

Inspeccioné la estructura del árbol binario en memoria con GDB y determiné que el número que cumplía con este camino era el 22. Al ingresar este valor, finalmente pude resolver la fase secreta.

---

## Conclusión general

Este trabajo implicó un enfoque metodológico utilizando análisis del ensamblador, inspección de memoria con GDB y automatización mediante scripts en Python. Cada fase presentó desafíos específicos y requirió solucionar pequeños errores y malentendidos iniciales mediante una cuidadosa revisión. La experiencia adquirida fue valiosa, especialmente por el aprendizaje en técnicas avanzadas de debugging y análisis binario.

