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
void print_error()
{
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// Inicializar el path por defecto
void init_path()
{
    search_path[0] = strdup("/bin");
    path_count = 1;
}

// Liberar memoria del path
void free_path()
{
    for (int i = 0; i < path_count; i++)
    {
        free(search_path[i]);
    }
    path_count = 0;
}

// Buscar ejecutable en el path
char *find_executable(char *cmd)
{
    static char full_path[256];

    for (int i = 0; i < path_count; i++)
    {
        snprintf(full_path, sizeof(full_path), "%s/%s", search_path[i], cmd);
        if (access(full_path, X_OK) == 0)
        {
            return full_path;
        }
    }
    return NULL;
}

// Comando built-in: exit
int builtin_exit(char **args)
{
    if (args[1] != NULL)
    {
        print_error();
        return -1;
    }
    exit(0);
}

// Comando built-in: cd
int builtin_cd(char **args)
{
    if (args[1] == NULL || args[2] != NULL)
    {
        print_error();
        return -1;
    }
    if (chdir(args[1]) != 0)
    {
        print_error();
        return -1;
    }
    return 0;
}

// Comando built-in: path
int builtin_path(char **args)
{
    // Liberar path anterior
    free_path();

    // Establecer nuevo path
    int i = 1;
    while (args[i] != NULL && i < MAX_PATH)
    {
        search_path[path_count] = strdup(args[i]);
        path_count++;
        i++;
    }
    return 0;
}

// Verificar si es comando built-in
int is_builtin(char **args)
{
    if (args[0] == NULL)
        return 0;

    if (strcmp(args[0], "exit") == 0)
    {
        return builtin_exit(args);
    }
    else if (strcmp(args[0], "cd") == 0)
    {
        return builtin_cd(args);
    }
    else if (strcmp(args[0], "path") == 0)
    {
        return builtin_path(args);
    }
    return -1; // No es built-in
}

// Ejecutar un comando simple
void execute_command(char **args, char *redirect_file)
{
    char *executable = find_executable(args[0]);

    if (executable == NULL)
    {
        print_error();
        exit(1);
    }

    // Manejar redirección si existe
    if (redirect_file != NULL)
    {
        int fd = open(redirect_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            print_error();
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    execv(executable, args);
    print_error();
    exit(1);
}

// Parsear y ejecutar un comando individual
void parse_and_execute(char *cmd)
{
    char *args[MAX_ARGS];
    char *redirect_file = NULL;
    int arg_count = 0;
    int redirect_count = 0;

    // Remover espacios al inicio y final
    while (*cmd == ' ' || *cmd == '\t')
        cmd++;

    // Parsear el comando
    char *token;
    char *rest = cmd;

    while ((token = strsep(&rest, " \t\n")) != NULL)
    {
        if (strlen(token) == 0)
            continue;

        if (strcmp(token, ">") == 0)
        {
            redirect_count++;
            if (redirect_count > 1)
            {
                print_error();
                return;
            }
            // El siguiente token es el archivo
            while ((token = strsep(&rest, " \t\n")) != NULL)
            {
                if (strlen(token) == 0)
                    continue;
                if (redirect_file != NULL)
                {
                    print_error();
                    return;
                }
                redirect_file = token;
                break;
            }
            continue;
        }

        if (arg_count >= MAX_ARGS - 1)
        {
            print_error();
            return;
        }
        args[arg_count++] = token;
    }
    args[arg_count] = NULL;

    // Verificar errores de redirección
    if (redirect_count > 0 && redirect_file == NULL)
    {
        print_error();
        return;
    }

    // Si no hay comando, retornar
    if (arg_count == 0)
        return;

    // Verificar si es built-in
    int builtin_result = is_builtin(args);
    if (builtin_result >= 0)
    {
        return; // Era built-in, ya se ejecutó
    }

    // Ejecutar comando externo
    pid_t pid = fork();
    if (pid == 0)
    {
        // Proceso hijo
        execute_command(args, redirect_file);
    }
    else if (pid > 0)
    {
        // Proceso padre - esperar al hijo
        waitpid(pid, NULL, 0);
    }
    else
    {
        print_error();
    }
}

// Procesar línea con comandos paralelos
void process_line(char *line)
{
    char *commands[MAX_ARGS];
    int cmd_count = 0;
    pid_t pids[MAX_ARGS];

    // Separar comandos por &
    char *token;
    char *rest = line;

    while ((token = strsep(&rest, "&")) != NULL)
    {
        if (strlen(token) == 0)
            continue;
        commands[cmd_count++] = token;
    }

    // Si no hay comandos paralelos, ejecutar normalmente
    if (cmd_count == 1)
    {
        parse_and_execute(commands[0]);
        return;
    }

    // Ejecutar comandos en paralelo
    for (int i = 0; i < cmd_count; i++)
    {
        char *args[MAX_ARGS];
        char *redirect_file = NULL;
        int arg_count = 0;
        int redirect_count = 0;

        char *cmd = commands[i];
        while (*cmd == ' ' || *cmd == '\t')
            cmd++;

        // Parsear comando
        char *tok;
        char *r = cmd;

        while ((tok = strsep(&r, " \t\n")) != NULL)
        {
            if (strlen(tok) == 0)
                continue;

            if (strcmp(tok, ">") == 0)
            {
                redirect_count++;
                if (redirect_count > 1)
                {
                    print_error();
                    goto cleanup;
                }
                while ((tok = strsep(&r, " \t\n")) != NULL)
                {
                    if (strlen(tok) == 0)
                        continue;
                    if (redirect_file != NULL)
                    {
                        print_error();
                        goto cleanup;
                    }
                    redirect_file = tok;
                    break;
                }
                continue;
            }
            args[arg_count++] = tok;
        }
        args[arg_count] = NULL;

        if (arg_count == 0)
            continue;

        // Verificar built-in (error en paralelo)
        if (is_builtin(args) >= 0)
        {
            continue;
        }

        // Fork para comando paralelo
        pid_t pid = fork();
        if (pid == 0)
        {
            execute_command(args, redirect_file);
        }
        else if (pid > 0)
        {
            pids[i] = pid;
        }
        else
        {
            print_error();
        }
    }

cleanup:
    // Esperar a todos los procesos hijos
    for (int i = 0; i < cmd_count; i++)
    {
        if (pids[i] > 0)
        {
            waitpid(pids[i], NULL, 0);
        }
    }
}

// Modo interactivo
void interactive_mode()
{
    char *line = NULL;
    size_t len = 0;

    while (1)
    {
        printf("wish> ");
        if (getline(&line, &len, stdin) == -1)
        {
            break;
        }
        process_line(line);
    }

    free(line);
}

// Modo batch
void batch_mode(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        print_error();
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, file) != -1)
    {
        process_line(line);
    }

    free(line);
    fclose(file);
}

int main(int argc, char *argv[])
{
    // Inicializar path
    init_path();

    // Verificar argumentos
    if (argc == 1)
    {
        // Modo interactivo
        interactive_mode();
    }
    else if (argc == 2)
    {
        // Modo batch
        batch_mode(argv[1]);
    }
    else
    {
        print_error();
        exit(1);
    }

    // Liberar memoria del path
    free_path();

    return 0;
}
