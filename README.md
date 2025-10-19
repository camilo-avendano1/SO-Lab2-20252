# Programa Wish Shell

## Información del Proyecto

**Participantes:**

- Juan Camilo Avendaño Rodriguez - CC: 1193585383
- Jose Fernando Waldo Rojas - CC: 1004807096

**Fecha:** Octubre 2025  
**Curso:** Sistemas Operativos  
**Práctica:** Laboratorio 2 - API de Procesos

## Descripción

`wish` (Wisconsin Shell) es una implementación simplificada de un shell de Unix que permite ejecutar comandos, gestionar procesos y realizar operaciones básicas del sistema. Este shell soporta comandos integrados (built-in), ejecución de comandos externos, redirección de salida y ejecución de comandos en paralelo.

## Funcionalidades

### Comandos Built-in

El shell incluye tres comandos integrados:

1. **exit**: Termina el shell
2. **cd**: Cambia el directorio actual de trabajo
3. **path**: Modifica las rutas de búsqueda de ejecutables

### Características Principales

- **Ejecución de comandos externos**: Busca y ejecuta programas en las rutas especificadas
- **Redirección de salida**: Permite redirigir stdout y stderr a archivos usando el operador `>`
- **Ejecución paralela**: Soporta ejecución simultánea de comandos usando el operador `&`
- **Modo interactivo**: Interfaz de línea de comandos con prompt `wish>`
- **Modo batch**: Ejecución de scripts desde archivos

## Compilación

Para compilar el programa, ejecute el siguiente comando en la terminal:

```bash
gcc -o wish wish.c -Wall
```

## Formas de Ejecución

### 1. Modo Interactivo

```bash
./wish
```

El shell mostrará el prompt `wish>` y esperará comandos del usuario.

**Ejemplo:**
```bash
wish> ls
wish> cd /tmp
wish> pwd
wish> exit
```

### 2. Modo Batch

```bash
./wish script.sh
```

Lee y ejecuta comandos desde el archivo especificado línea por línea.

**Ejemplo de archivo `script.sh`:**
```bash
ls -l
cd /tmp
pwd
```

## Ejemplos de Uso

### Comandos Básicos

```bash
wish> ls -l
wish> pwd
wish> cat archivo.txt
```

### Cambio de Directorio

```bash
wish> cd /home/usuario
wish> cd ..
```

### Gestión de Path

```bash
wish> path /bin /usr/bin
wish> path /usr/local/bin
wish> path
```

El último comando vacía el path, deshabilitando la ejecución de comandos externos.

### Redirección de Salida

```bash
wish> ls -l > salida.txt
wish> cat archivo.txt > copia.txt
wish> grep "patron" archivo.txt > resultados.txt
```

**Nota**: La redirección redirige tanto stdout como stderr al mismo archivo.

### Ejecución Paralela

```bash
wish> ls & pwd & whoami
wish> sleep 5 & sleep 3 & echo "Hola"
```

Los comandos separados por `&` se ejecutan simultáneamente. El shell espera a que todos terminen antes de mostrar el siguiente prompt.

### Combinación de Características

```bash
wish> ls -l > archivo1.txt & pwd > archivo2.txt & whoami > archivo3.txt
```

## Arquitectura del Programa

### Estructura de Datos Principal

#### Array de Rutas de Búsqueda
```c
char *search_path[MAX_PATH];  // Arreglo de rutas
int path_count = 0;            // Contador de rutas activas
```

Esta estructura permite:
- Almacenar múltiples rutas de búsqueda
- Modificar dinámicamente las rutas con el comando `path`
- Buscar ejecutables en orden secuencial

### Flujo de Ejecución Paso a Paso

#### 1. Inicialización

- Se establece el path por defecto a `/bin`
- Se verifica el número de argumentos (modo interactivo vs batch)
- Se inicializan las estructuras de datos necesarias

#### 2. Lectura de Entrada

**Modo Interactivo:**
- Muestra el prompt `wish>`
- Lee línea de entrada con `getline()`
- Continúa hasta recibir EOF (Ctrl+D)

**Modo Batch:**
- Abre el archivo especificado
- Lee línea por línea
- Procesa cada comando secuencialmente

#### 3. Procesamiento de Línea

1. **Detección de comandos paralelos**: Separa la línea por el operador `&`
2. **Parsing de cada comando**:
   - Tokeniza por espacios y tabulaciones
   - Identifica operadores de redirección `>`
   - Extrae argumentos y archivo de salida

#### 4. Ejecución de Comandos

**Para comandos built-in:**
- Se ejecutan directamente en el proceso padre
- No se permite su uso en comandos paralelos
- Retornan inmediatamente después de ejecutarse

**Para comandos externos:**
1. Se busca el ejecutable en el path
2. Se crea un proceso hijo con `fork()`
3. En el hijo:
   - Se configura la redirección si existe
   - Se ejecuta el comando con `execv()`
4. El padre espera con `waitpid()`

#### 5. Ejecución Paralela

Cuando hay múltiples comandos separados por `&`:
1. Se crean múltiples procesos hijos simultáneamente
2. Se guarda el PID de cada proceso
3. Se esperan todos los procesos con `waitpid()` en bucle

#### 6. Limpieza

- Se liberan todas las rutas del path
- Se cierran archivos abiertos
- Se libera memoria de buffers

## Funciones Clave Utilizadas

### Funciones del Sistema

#### `fork()`

```c
pid_t fork(void);
```

- **Propósito**: Crear un proceso hijo idéntico al padre
- **Retorno**: 
  - `0` en el proceso hijo
  - PID del hijo en el proceso padre
  - `-1` en caso de error
- **Uso en el programa**: Ejecutar comandos externos en procesos separados

#### `execv()`

```c
int execv(const char *path, char *const argv[]);
```

- **Propósito**: Reemplazar el proceso actual con un nuevo programa
- **Parámetros**:
  - `path`: Ruta completa al ejecutable
  - `argv`: Array de argumentos terminado en NULL
- **Uso en el programa**: Ejecutar comandos externos después del fork

#### `waitpid()`

```c
pid_t waitpid(pid_t pid, int *status, int options);
```

- **Propósito**: Esperar a que un proceso hijo termine
- **Uso en el programa**: 
  - Sincronizar ejecución de comandos
  - Esperar procesos paralelos

#### `getline()`

```c
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
```

- **Propósito**: Leer una línea completa de entrada
- **Ventaja**: Maneja líneas de cualquier longitud
- **Uso en el programa**: Leer comandos del usuario o archivo batch

#### `access()`

```c
int access(const char *pathname, int mode);
```

- **Propósito**: Verificar permisos de archivo
- **Modo usado**: `X_OK` (ejecutable)
- **Uso en el programa**: Buscar ejecutables en las rutas del path

#### `chdir()`

```c
int chdir(const char *path);
```

- **Propósito**: Cambiar el directorio de trabajo actual
- **Uso en el programa**: Implementar el comando built-in `cd`

#### `open()`, `dup2()`, `close()`

```c
int open(const char *pathname, int flags, mode_t mode);
int dup2(int oldfd, int newfd);
int close(int fd);
```

- **Propósito**: Manejar redirección de entrada/salida
- **Uso en el programa**:
  - Abrir archivo de salida
  - Redirigir stdout y stderr al archivo
  - Cerrar descriptores innecesarios

#### `strdup()`, `strsep()`

```c
char *strdup(const char *s);
char *strsep(char **stringp, const char *delim);
```

- **Propósito**: Manipulación de cadenas
- **Uso en el programa**:
  - Copiar rutas del path
  - Tokenizar comandos y argumentos

### Funciones Personalizadas

#### `init_path()`

```c
void init_path();
```

- **Propósito**: Inicializar el path por defecto
- **Funcionamiento**: Establece `/bin` como única ruta inicial

#### `free_path()`

```c
void free_path();
```

- **Propósito**: Liberar memoria de todas las rutas
- **Funcionamiento**: Recorre el array y libera cada cadena

#### `find_executable()`

```c
char *find_executable(char *cmd);
```

- **Propósito**: Buscar un ejecutable en el path
- **Funcionamiento**:
  1. Concatena cada ruta con el nombre del comando
  2. Verifica con `access()` si es ejecutable
  3. Retorna la primera coincidencia o NULL

#### `builtin_exit()`

```c
int builtin_exit(char **args);
```

- **Propósito**: Implementar el comando `exit`
- **Validación**: No acepta argumentos
- **Resultado**: Termina el shell con `exit(0)`

#### `builtin_cd()`

```c
int builtin_cd(char **args);
```

- **Propósito**: Implementar el comando `cd`
- **Validación**: Requiere exactamente un argumento
- **Funcionamiento**: Usa `chdir()` para cambiar directorio

#### `builtin_path()`

```c
int builtin_path(char **args);
```

- **Propósito**: Modificar las rutas de búsqueda
- **Funcionamiento**:
  1. Libera el path actual
  2. Establece nuevo path con los argumentos recibidos
  3. Path vacío si no hay argumentos

#### `is_builtin()`

```c
int is_builtin(char **args);
```

- **Propósito**: Detectar y ejecutar comandos built-in
- **Retorno**:
  - `0` o valor positivo si es built-in y se ejecutó
  - `-1` si no es built-in

#### `execute_command()`

```c
void execute_command(char **args, char *redirect_file);
```

- **Propósito**: Ejecutar un comando externo en proceso hijo
- **Funcionamiento**:
  1. Busca el ejecutable con `find_executable()`
  2. Configura redirección si se especificó
  3. Ejecuta con `execv()`
  4. Maneja errores y termina proceso

#### `parse_and_execute()`

```c
void parse_and_execute(char *cmd);
```

- **Propósito**: Parsear y ejecutar un comando individual
- **Funcionamiento**:
  1. Tokeniza el comando
  2. Detecta operador de redirección
  3. Valida errores de sintaxis
  4. Ejecuta built-in o crea fork para comando externo

#### `process_line()`

```c
void process_line(char *line);
```

- **Propósito**: Procesar una línea completa con posibles comandos paralelos
- **Funcionamiento**:
  1. Separa comandos por `&`
  2. Si es un solo comando, usa `parse_and_execute()`
  3. Si hay varios, crea múltiples forks y espera a todos

#### `interactive_mode()`

```c
void interactive_mode();
```

- **Propósito**: Implementar el modo interactivo
- **Funcionamiento**:
  - Muestra prompt `wish>`
  - Lee comandos con `getline()`
  - Procesa cada línea hasta EOF

#### `batch_mode()`

```c
void batch_mode(char *filename);
```

- **Propósito**: Implementar el modo batch
- **Funcionamiento**:
  - Abre archivo especificado
  - Lee y procesa línea por línea
  - Maneja errores de archivo

## Manejo de Errores

El programa implementa un robusto manejo de errores estandarizado:

### Mensaje de Error Único

```c
char error_message[30] = "An error has occurred\n";
```

Todos los errores muestran el mismo mensaje en `stderr` siguiendo la especificación del proyecto.

### Casos de Error Manejados

1. **Comando `exit` con argumentos**:
   ```bash
   wish> exit ahora
   An error has occurred
   ```

2. **Comando `cd` sin argumentos o con múltiples argumentos**:
   ```bash
   wish> cd
   An error has occurred
   wish> cd /tmp /home
   An error has occurred
   ```

3. **Comando `cd` con directorio inválido**:
   ```bash
   wish> cd /directorio/inexistente
   An error has occurred
   ```

4. **Comando no encontrado en el path**:
   ```bash
   wish> comando_inexistente
   An error has occurred
   ```

5. **Error en redirección**:
   - Múltiples operadores `>` en un comando
   - Operador `>` sin archivo de destino
   - Múltiples archivos después de `>`
   - Error al abrir archivo de salida

6. **Error al abrir archivo batch**:
   ```bash
   ./wish archivo_inexistente.sh
   An error has occurred
   ```

7. **Argumentos incorrectos al invocar wish**:
   ```bash
   ./wish archivo1 archivo2 archivo3
   An error has occurred
   ```

8. **Errores de sistema**:
   - Fallo en `fork()`
   - Fallo en `execv()`

## Consideraciones de Implementación

### Manejo de Memoria

- Se usa `strdup()` para copiar dinámicamente las rutas del path
- Toda memoria reservada se libera con `free_path()` al terminar
- `getline()` gestiona automáticamente el buffer de entrada

### Búsqueda de Ejecutables

- Se busca secuencialmente en todas las rutas del path
- Se usa `access()` con flag `X_OK` para verificar permisos de ejecución
- Se retorna la primera coincidencia encontrada

### Redirección

- Tanto stdout (descriptor 1) como stderr (descriptor 2) se redirigen al mismo archivo
- Se usa modo `O_WRONLY | O_CREAT | O_TRUNC` para sobrescribir archivos
- Permisos del archivo: 0644 (rw-r--r--)

### Comandos Paralelos

- Se crean todos los procesos hijos antes de esperar a cualquiera
- Se usa `waitpid()` con el PID específico para cada proceso
- Los built-ins en comandos paralelos se ignoran silenciosamente

### Parsing

- Se usa `strsep()` para tokenizar por espacios y tabulaciones
- Los tokens vacíos se saltan automáticamente
- Se preservan los espacios dentro de rutas (aunque no se soportan comillas)

## Limitaciones Conocidas

1. **Sin soporte para comillas**: No se pueden usar espacios en nombres de archivo
2. **Sin pipes**: No soporta el operador `|` para encadenar comandos
3. **Sin variables de entorno**: No se pueden usar ni modificar variables como `$HOME`
4. **Sin expansión de comodines**: No expande `*`, `?`, etc.
5. **Sin manejo de señales**: No captura Ctrl+C ni otras señales
6. **Path limitado**: Máximo `MAX_PATH` (64) rutas

## Pruebas Recomendadas

### Casos Básicos

```bash
# Comandos simples
wish> ls
wish> pwd
wish> whoami

# Built-ins
wish> cd /tmp
wish> path /bin /usr/bin
wish> exit
```

### Casos de Redirección

```bash
wish> ls > output.txt
wish> cat archivo.txt > copia.txt
wish> ls > out.txt & pwd > out2.txt
```

### Casos Paralelos

```bash
wish> sleep 2 & sleep 1 & echo "done"
wish> ls & pwd & whoami
```

### Casos de Error

```bash
wish> exit con argumentos
wish> cd
wish> cd /tmp /home
wish> comando_inexistente
wish> ls > > archivo.txt
wish> ls >
wish> ls > file1 file2
```

### Modo Batch

Crear archivo `test.sh`:
```bash
ls -l
pwd
cd /tmp
pwd
path /bin /usr/bin
ls
exit
```

Ejecutar:
```bash
./wish test.sh
```

### Casos Límite

```bash
# Path vacío
wish> path
wish> ls
An error has occurred

# Múltiples comandos paralelos
wish> cmd1 & cmd2 & cmd3 & cmd4 & cmd5

# Líneas vacías (deben ignorarse)
wish> 

wish> ls
```

## Diferencias con Shells Reales

- **Bash/Zsh** soportan características avanzadas como:
  - Expansión de variables
  - Sustitución de comandos
  - Control de trabajos (bg, fg)
  - Historial de comandos
  - Auto-completado
  - Scripts complejos con condicionales y bucles

- **wish** es una versión educativa simplificada enfocada en:
  - Conceptos fundamentales de procesos
  - API básica de procesos Unix (fork, exec, wait)
  - Manejo básico de archivos y redirección
