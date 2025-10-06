#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS 64
#define MAX_PATH 64

// Variable global para el path
char *search_path[MAX_PATH];
int path_count = 0;

// Mensaje de error estándar
char error_message[30] = "An error has occurred\n";

// Función para imprimir error
void print_error() {
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// Inicializar el path por defecto
void init_path() {
    search_path[0] = strdup("/bin");
    path_count = 1;
}

// Liberar memoria del path
void free_path() {
    for (int i = 0; i < path_count; i++) {
        free(search_path[i]);
    }
    path_count = 0;
}

// Buscar ejecutable en el path
char* find_executable(char *cmd) {
    static char full_path[256];
    
    for (int i = 0; i < path_count; i++) {
        snprintf(full_path, sizeof(full_path), "%s/%s", search_path[i], cmd);
        if (access(full_path, X_OK) == 0) {
            return full_path;
        }
    }
    return NULL;
}

// Comando built-in: exit
int builtin_exit(char **args) {
    if (args[1] != NULL) {
        print_error();
        return -1;
    }
    exit(0);
}

// Comando built-in: cd
int builtin_cd(char **args) {
    if (args[1] == NULL || args[2] != NULL) {
        print_error();
        return -1;
    }
    if (chdir(args[1]) != 0) {
        print_error();
        return -1;
    }
    return 0;
}
