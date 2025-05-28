#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_LINE   1024
#define MAX_CMDS   200
#define MAX_ARGS   64  // número máximo de argumentos válidos (índices 0..63)

char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

void read_line(char *buf) {
    if (!fgets(buf, MAX_LINE, stdin)) exit(0);
    buf[strcspn(buf, "\n")] = '\0';
}

int bad_syntax(const char *line) {
    int len = strlen(line);
    if (len == 0 || line[0] == '|' || line[len-1] == '|') return 1;
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

int split_pipes(char *line, char **commands) {
    int count = 0;
    for (char *tok = strtok(line, "|"); tok && count < MAX_CMDS; tok = strtok(NULL, "|")) {
        while (isspace((unsigned char)*tok)) tok++;
        commands[count++] = strdup(tok);
    }
    return count;
}

void free_args(char **args) {
    if (!args) return;
    for (int i = 0; args[i]; ++i) free(args[i]);
    free(args);
}

char **split_args(char *cmd) {
    // Reservo MAX_ARGS+1 para el NULL final
    char **argv = calloc(MAX_ARGS + 1, sizeof(char*));
    int i = 0;

    while (*cmd) {
        // Saltar espacios
        while (isspace((unsigned char)*cmd)) cmd++;
        if (!*cmd) break;

        // Si ya tengo MAX_ARGS argumentos, error
        if (i >= MAX_ARGS) {
            fprintf(stderr, "Too many arguments\n");
            for (int k = 0; k < i; ++k) free(argv[k]);
            free(argv);
            return NULL;
        }

        // Determinar token
        char *start;
        if (*cmd == '"') {
            cmd++;
            start = cmd;
            while (*cmd && *cmd != '"') cmd++;
        } else {
            start = cmd;
            while (*cmd && !isspace((unsigned char)*cmd)) cmd++;
        }

        // Cerrar token
        if (*cmd) *cmd++ = '\0';
        argv[i++] = strdup(start);
    }
    argv[i] = NULL;
    return argv;
}

void run_pipeline(char ***args_list, int cmd_count) {
    int in_fd = STDIN_FILENO, fd[2];
    pid_t pids[MAX_CMDS];

    for (int i = 0; i < cmd_count; ++i) {
        if (i < cmd_count - 1 && pipe(fd) < 0) { perror("pipe"); exit(1); }

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }

        if (pid == 0) {  // Proceso hijo
            if (in_fd != STDIN_FILENO) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < cmd_count - 1) {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }
            execvp(args_list[i][0], args_list[i]);
            fprintf(stderr, "command not found\n");
            free_args(args_list[i]);
            exit(1);
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


void free_resources(char ***args_list, char **commands, int cmd_count) {
    for (int i = 0; i < cmd_count; ++i) {
        if (args_list[i]) {
            for (int j = 0; args_list[i][j]; ++j) free(args_list[i][j]);
            free(args_list[i]);
        }
        free(commands[i]);
    }
    free(args_list);
}

int main(void) {
    char line_buf[MAX_LINE];
    char *commands[MAX_CMDS];

    while (1) {
        printf("Shell> "); fflush(stdout);
        read_line(line_buf);
        char *line = trim(line_buf);
        if (strcmp(line, "exit") == 0) break;
        if (bad_syntax(line)) {
            fprintf(stderr, "Syntax error\n");
            continue;
        }

        int cmd_count = split_pipes(line, commands);
        if (!cmd_count) continue;

        char ***args_list = malloc(cmd_count * sizeof(char**));
        int valid = 1;
        for (int i = 0; i < cmd_count; ++i) {
            args_list[i] = split_args(commands[i]);
            if (!args_list[i]) {
                // fallo por demasiados args: limpiar todo
                valid = 0;
                for (int k = 0; k < i; ++k) free_args(args_list[k]);
                for (int k = 0; k <= i; ++k) free(commands[k]);
                free(args_list);
                break;
            }
        }
        if (!valid) continue;

        run_pipeline(args_list, cmd_count);
        free_resources(args_list, commands, cmd_count);
    }
    return 0;
}
