#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_LINEA 1024
#define MAX_COMANDOS 200
#define MAX_ARGS 64

// Elimina espacios iniciales y finales de una string
char *trim(char *s) {
    while (isspace(*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace(*end)) *end-- = '\0';
    return s;
}

// Lee una línea desde stdin y elimina el \n final
void leer_linea(char *buffer, size_t size) {
    if (fgets(buffer, size, stdin) == NULL) {
        perror("fgets");
        exit(EXIT_FAILURE);
    }
    buffer[strcspn(buffer, "\n")] = '\0';
}

// Divide una línea en subcomandos separados por pipes
int dividir_por_pipes(char *linea, char **comandos) {
    int count = 0;
    char *token = strtok(linea, "|");
    while (token != NULL && count < MAX_COMANDOS) {
        while (isspace(*token)) token++;
        comandos[count++] = strdup(token);
        token = strtok(NULL, "|");
    }
    return count;
}

// Divide un comando en argumentos, respetando comillas dobles
char **tokenizar_comando(char *cmd) {
    char **argv = calloc(MAX_ARGS, sizeof(char *));
    int j = 0;

    while (*cmd != '\0') {
        while (isspace(*cmd)) cmd++;
        if (*cmd == '\0') break;

        char *arg;
        if (*cmd == '"') {
            cmd++;
            arg = cmd;
            while (*cmd && *cmd != '"') cmd++;
        } else {
            arg = cmd;
            while (*cmd && !isspace(*cmd)) cmd++;
        }

        if (*cmd) *cmd++ = '\0';
        argv[j++] = strdup(arg);
    }
    argv[j] = NULL;
    return argv;
}

// Ejecuta una secuencia de comandos conectados por pipes
void ejecutar_pipeline(char ***argvs, int num_comandos) {
    int in_fd = 0, fd[2];

    for (int i = 0; i < num_comandos; i++) {
        if (i < num_comandos - 1 && pipe(fd) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            if (i > 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < num_comandos - 1) {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }
            execvp(argvs[i][0], argvs[i]);
            fprintf(stderr, "%s: comando no encontrado\n", argvs[i][0]);
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        if (i > 0) close(in_fd);
        if (i < num_comandos - 1) {
            close(fd[1]);
            in_fd = fd[0];
        }
    }

    for (int i = 0; i < num_comandos; i++) {
        wait(NULL);
    }
}

// Libera la memoria reservada para los argumentos y comandos
void liberar_memoria(char ***argvs, char **comandos, int num_comandos) {
    for (int i = 0; i < num_comandos; i++) {
        for (int j = 0; argvs[i][j]; j++) {
            free(argvs[i][j]);
        }
        free(argvs[i]);
        free(comandos[i]);
    }
    free(argvs);
}

int main() {
    char buffer[MAX_LINEA];
    char *comandos[MAX_COMANDOS];

    while (1) {
        printf("Shell> ");
        fflush(stdout);

        leer_linea(buffer, sizeof(buffer));
        char *linea = trim(buffer);

        if (strcmp(linea, "exit") == 0) break;

        // Validación básica de sintaxis
        if (linea[0] == '|' || linea[strlen(linea) - 1] == '|') {
            fprintf(stderr, "Error de sintaxis: pipe al inicio o al final\n");
            continue;
        }
        if (strstr(linea, "||")) {
            fprintf(stderr, "Error de sintaxis: pipes duplicados sin comando entre medio\n");
            continue;
        }

        int num_comandos = dividir_por_pipes(linea, comandos);
        if (num_comandos == 0) continue;

        char ***argvs = malloc(num_comandos * sizeof(char **));
        for (int i = 0; i < num_comandos; i++) {
            argvs[i] = tokenizar_comando(comandos[i]);
        }

        ejecutar_pipeline(argvs, num_comandos);
        liberar_memoria(argvs, comandos, num_comandos);
    }

    return 0;
}
