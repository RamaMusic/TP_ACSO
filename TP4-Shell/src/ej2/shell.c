#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LINE 8192
#define MAX_CMDS 200
#define MAX_ARGS 64

volatile sig_atomic_t shell_running = 1;

// ─────────────────────────────────────────────────────────────────────────────
// Utils
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Elimina espacios en blanco del inicio y final de una cadena.
 */
char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

/**
 * Detecta errores de sintaxis en la línea, como pipes al inicio o final,
 * dobles pipes, o secuencias inválidas.
 */
int is_syntax_error(const char *line) {
    int len = strlen(line);
    if (len == 0 || line[0] == '|' || line[len - 1] == '|') return 1;
    if (strstr(line, "||")) return 1;
    for (int i = 0; i < len - 1; ++i) {
        if (line[i] == '|') {
            int j = i + 1;
            while (j < len && isspace((unsigned char)line[j])) j++;
            if (j < len && line[j] == '|') return 1;
        }
    }
    return 0;
}

/**
 * Captura señales como SIGINT o SIGTERM y prepara el cierre del shell.
 */
void signal_handler(int sig) {
    (void)sig;
    shell_running = 0;
    const char *msg = "\nShell shutting down...\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

/**
 * Configura los manejadores de señal necesarios para interrumpir
 * la ejecución del shell de forma segura.
 */
void setup_signals(void) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

// ─────────────────────────────────────────────────────────────────────────────
// Parsing
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Separa una línea de entrada en múltiples comandos divididos por el pipe '|',
 * ignorando los pipes que estén dentro de comillas dobles.
 */
int split_pipeline(char *line, char **commands) {
    int count = 0;
    char *start = line;
    int in_quote = 0;

    for (char *p = line; ; ++p) {
        if (*p == '"') {
            in_quote = !in_quote;
        } else if (*p == '|' && !in_quote) {
            *p = '\0';
            commands[count++] = strdup(trim(start));
            start = p + 1;
            if (count >= MAX_CMDS) break;
        } else if (*p == '\0') {
            commands[count++] = strdup(trim(start));
            break;
        }
    }

    return count;
}

/**
 * Parsea un comando individual y construye un array de argumentos,
 * respetando los grupos entre comillas.
 */
char **parse_args(char *cmd) {
    char **argv = calloc(MAX_ARGS + 1, sizeof(char *));
    int i = 0;

    while (*cmd) {
        while (isspace((unsigned char)*cmd)) cmd++;
        if (!*cmd) break;

        if (i >= MAX_ARGS) {
            fprintf(stderr, "Too many arguments\n");
            for (int k = 0; k < i; ++k) free(argv[k]);
            free(argv);
            return NULL;
        }

        char *start;
        int quoted = 0;

        if (*cmd == '"') {
            quoted = 1;
            cmd++;
            start = cmd;
            while (*cmd && *cmd != '"') cmd++;

            if (!*cmd) {
                fprintf(stderr, "Syntax error: missing closing quote\n");
                for (int k = 0; k < i; ++k) free(argv[k]);
                free(argv);
                return NULL;
            }
        } else {
            start = cmd;
            while (*cmd && !isspace((unsigned char)*cmd)) cmd++;
        }

        char saved = *cmd;
        *cmd = '\0';
        argv[i++] = strdup(start);
        if (quoted && saved == '"') cmd++;
        else if (!quoted && saved) cmd++;
    }

    argv[i] = NULL;
    return argv;
}

/**
 * Libera la memoria asignada para un array de argumentos.
 */
void free_args(char **args) {
    if (!args) return;
    for (int i = 0; args[i]; ++i) free(args[i]);
    free(args);
}

// ─────────────────────────────────────────────────────────────────────────────
// Ejecución
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Ejecuta una lista de comandos conectados por pipes,
 * creando procesos hijo y redireccionando entradas/salidas.
 */
void run_pipeline(char ***args_list, int cmd_count) {
    int in_fd = STDIN_FILENO, fd[2];
    pid_t pids[MAX_CMDS];

    for (int i = 0; i < cmd_count; ++i) {
        if (i < cmd_count - 1 && pipe(fd) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            if (in_fd != STDIN_FILENO) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < cmd_count - 1) {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }
            if (strcmp(args_list[i][0], "exit") == 0) {
                exit(0);
            }
            execvp(args_list[i][0], args_list[i]);
            fprintf(stderr, "command not found\n");
            exit(EXIT_FAILURE);
        }

        pids[i] = pid;

        if (in_fd != STDIN_FILENO) close(in_fd);
        if (i < cmd_count - 1) {
            close(fd[1]);
            in_fd = fd[0];
        }
    }

    for (int i = 0; i < cmd_count; ++i) {
        int status;
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
        }
    }
}

/**
 * Libera todos los recursos usados por los comandos y argumentos en una ejecución.
 */
void free_all(char ***args_list, char **commands, int cmd_count) {
    for (int i = 0; i < cmd_count; ++i) {
        free_args(args_list[i]);
        free(commands[i]);
    }
    free(args_list);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────

int main(void) {
    char line_buf[MAX_LINE];
    char *commands[MAX_CMDS];

    setup_signals();
    if (isatty(STDIN_FILENO)) {
        printf("Shell started. Type 'exit' to quit.\n");
    }

    while (shell_running) {
        if (isatty(STDIN_FILENO)) {
            printf("Shell> ");
        }
        fflush(stdout);
        if (!fgets(line_buf, sizeof(line_buf), stdin)) break;

        char *line = trim(line_buf);
        if (strcmp(line, "exit") == 0) break;
        if (is_syntax_error(line)) {
            fprintf(stderr, "Syntax error\n");
            continue;
        }

        int cmd_count = split_pipeline(line, commands);
        if (!cmd_count) continue;

        char ***args_list = malloc(cmd_count * sizeof(char**));
        int valid = 1;
        for (int i = 0; i < cmd_count; ++i) {
            args_list[i] = parse_args(commands[i]);
            if (!args_list[i]) {
                valid = 0;
                break;
            }
        }
        if (valid) run_pipeline(args_list, cmd_count);
        free_all(args_list, commands, cmd_count);
    }

    if (isatty(STDIN_FILENO)) {
        printf("Shell terminated.\n");
    }
    return 0;
}