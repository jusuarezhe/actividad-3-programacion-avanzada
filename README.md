# Proyecto de Programación Orientada a Objetos – C++

Este repositorio contiene los programas desarrollados para las actividades del curso.  
Cada programa está ubicado en un archivo independiente:

- `codigo1.cpp`
- `codigo2.cpp`
- `codigo3.cpp`

Todos los programas pueden compilarse y ejecutarse desde cualquier terminal, incluyendo el terminal integrado de Visual Studio Code.

---

## Instrucciones para Compilar y Ejecutar

En Windows (usando **g++**):

1. Abrir la terminal (CMD o PowerShell) en la carpeta donde están los archivos `.cpp`.
2. Compilar con el comando:  
g++ codigoX.cpp -o codigoX.exe
3. Ejecutar el archivo con: 
codigoX.exe
4. Cambiar `X` por el número (1, 2 o 3) según el programa que quiera ejecutar.  
Cada programa es independiente y se ejecuta por separado.

---

# 📌 código1.cpp — Lista Enlazada de Estudiantes

### **Metodología**
- Implementación de una **lista enlazada simple** para almacenar estudiantes.
- Cada nodo contiene:
- Nombre del estudiante
- Código/ID
- Nota
- Funciones para:
- Insertar estudiantes
- Mostrar la lista completa
- Buscar por código
- Eliminar un estudiante
- Uso de memoria dinámica para administrar nodos.

### **Objetivo**
Comprender el manejo de listas enlazadas, punteros, memoria dinámica y operaciones CRUD básicas en estructuras dinámicas.

---

# 📌 código2.cpp — Radiografía (Procesamiento básico de imágenes en matriz)

### **Metodología**
- Implementación de una **matriz dinámica** para simular una radiografía.
- Funciones para:
- Crear la matriz
- Llenarla con valores
- Detectar "fracturas" según valores umbral
- Imprimir la radiografía en formato matricial
- Manejo completo de memoria dinámica y validaciones de entrada.

### **Objetivo**
Aplicar memoria dinámica bidimensional, manipulación de datos matriciales y detección de patrones dentro de una estructura tipo imagen.

---

# 📌 código3.cpp — Juego de Dominó con Historial (POO + Listas Enlazadas)

### **Metodología**
- Basado en el ejercicio previo del juego de dominó.
- Modificado para incluir:
- Una **lista enlazada dinámica** que almacena el historial de movimientos.
- Cada nodo registra:
 - Nombre del jugador
 - Ficha jugada
 - Estado del tablero después del movimiento
- Al finalizar la partida, se genera un archivo:  
**`historial_domino.txt`**
- Clases utilizadas:
- `Ficha`
- `Jugador`
- `JuegoDomino`
- `Historial` (lista enlazada de movimientos)
- Integración fluida con la lógica original del juego.

### **Objetivo**
Reforzar el uso de clases, listas enlazadas, estructuras dinámicas y escritura de archivos para registrar el historial del juego.

---

## Instrucciones de Uso de los Programas

### **codigo1.cpp**
- El programa pedirá información de estudiantes.
- Ingrese los datos tal como se solicitan.
- Puede agregar, listar, buscar y eliminar estudiantes.

### **codigo2.cpp**
- Ingrese dimensiones válidas para la matriz.
- La "radiografía" mostrará valores que simulan intensidad.
- Se mostrarán alertas si se detectan patrones irregulares.

### **codigo3.cpp — Juego de Dominó**
- Al ejecutar, aparece un menú inicial.
- Configure jugadores antes de iniciar la partida.
- Juegue normalmente; el historial se irá almacenando.
- Al terminar, revise el archivo `historial_domino.txt`.

---

# Autores

Juan Suárez Herron  
Santiago Torres Guerrero  
Marbin Fabián Rivero  

Año: **2025**
