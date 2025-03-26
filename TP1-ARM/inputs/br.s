.section .text
.global _start

_start:
    // Simulamos una llamada a función
    BL funcion           // Salta y guarda PC+4 en X30

    // Esto se ejecuta cuando regresamos de la función
    ADDS X1, X0, #1      // X1 = 1
    HLT #0

// Función que se salta
funcion:
    ADDS X2, X0, #2      // X2 = 2 (se ejecuta si BL funcionó)
    BR X30               // Volver a la instrucción después del BL
