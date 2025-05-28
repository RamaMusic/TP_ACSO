#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_LINE   1024
#define MAX_CMDS   200
#define MAX_ARGS   64

// Elimina espacios al inicio y final in-place
char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

// Lee una línea desde stdin
void read_line(char *buf) {
    if (!fgets(buf, MAX_LINE, stdin)) exit(0);
    buf[strcspn(buf, "\n")] = '\0';
}

// Verifica errores de sintaxis básicos
int bad_syntax(const char *line) {
    int len = strlen(line);
    return len == 0 || line[0] == '|' || line[len-1] == '|' || strstr(line, "||");
}

// Separa la línea por '|' en comandos individuales
int split_pipes(char *line, char **commands) {
    int count = 0;
    for (char *tok = strtok(line, "|"); tok && count < MAX_CMDS; tok = strtok(NULL, "|")) {
        while (isspace((unsigned char)*tok)) tok++;
        commands[count++] = strdup(tok);
    }
    return count;
}

// Divide un comando en argumentos, respetando comillas
char **split_args(char *cmd) {
    char **argv = calloc(MAX_ARGS, sizeof(char*));
    int i = 0;
    while (*cmd) {
        while (isspace((unsigned char)*cmd)) cmd++;
        if (!*cmd) break;

        char *start;
        if (*cmd == '"') {
            cmd++;
            start = cmd;
            while (*cmd && *cmd != '"') cmd++;
        } else {
            start = cmd;
            while (*cmd && !isspace((unsigned char)*cmd)) cmd++;
        }

        if (*cmd) *cmd++ = '\0';
        argv[i++] = strdup(start);
    }
    argv[i] = NULL;
    return argv;
}

// Ejecuta la lista de comandos conectados por pipes
void run_pipeline(char ***args_list, int cmd_count) {
    int in_fd = STDIN_FILENO, fd[2];

    for (int i = 0; i < cmd_count; ++i) {
        if (i < cmd_count - 1 && pipe(fd) < 0) { perror("pipe"); exit(1); }

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }

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
            execvp(args_list[i][0], args_list[i]);
            perror("execvp");
            exit(1);
        }

        if (in_fd != STDIN_FILENO) close(in_fd);
        if (i < cmd_count - 1) {
            close(fd[1]);
            in_fd = fd[0];
        }
    }

    while (wait(NULL) > 0);
}

// Libera memoria usada
void free_resources(char ***args_list, char **commands, int cmd_count) {
    for (int i = 0; i < cmd_count; ++i) {
        for (int j = 0; args_list[i][j]; ++j) free(args_list[i][j]);
        free(args_list[i]);
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
        for (int i = 0; i < cmd_count; ++i)
            args_list[i] = split_args(commands[i]);

        run_pipeline(args_list, cmd_count);
        free_resources(args_list, commands, cmd_count);
    }

    return 0;
}
