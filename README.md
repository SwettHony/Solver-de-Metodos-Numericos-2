# Solver-de-Metodos-Numericos-2

# Métodos Numéricos — Derivación e Integración

Programa en C++ que implementa métodos numéricos de derivación e integración, con un **evaluador de expresiones matemáticas propio** (parser recursivo descendente) que permite ingresar cualquier función `f(x)` como texto.

## Temas implementados

**3.1 Derivación numérica**
- Diferencia hacia adelante y hacia atrás — O(h)
- Diferencia central — O(h²)
- Segunda derivada central — O(h²)
- Derivada de orden superior con extrapolación de Richardson

**3.2 Fórmulas de Newton-Cotes**
- Regla del Trapecio
- Regla de Simpson 1/3
- Regla de Simpson 3/8

**3.3 Integración de Romberg** (con tabla de extrapolación y control de tolerancia)

## Evaluador de expresiones

Se puede ingresar `f(x)` directamente como texto, por ejemplo:

```
x^3 - 2*x + sin(x)
```

Soporta: `+ - * / ^`, paréntesis, y las funciones `sin, cos, tan, asin, acos, atan, sinh, cosh, tanh, exp, log, ln, sqrt, abs`, además de las constantes `pi` y `e`.

## Compilación

```bash
g++ solver_metodos_numericos.cpp -o solver -lm
./solver
```

## Uso

El programa despliega un menú interactivo donde eliges el método, ingresas la función y los parámetros (intervalo, número de subintervalos, tolerancia, etc.), y muestra el resultado junto con una tabla de los puntos/iteraciones usados.
