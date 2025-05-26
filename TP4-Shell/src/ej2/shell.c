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

// Elimina espacios en blanco al inicio y al final in-place
// Retorna un puntero al primer caracter que no es espacio
char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

// Lee una línea de stdin y la guarda en buf (longitud máxima MAX_LINE)
void read_line(char *buf) {
    if (!fgets(buf, MAX_LINE, stdin)) exit(0);
    buf[strcspn(buf, "\n")] = '\0';
}

// Devuelve 1 si hay error de sintaxis: pipe al principio/final o si hay "||"
int bad_syntax(const char *line) {
    int len = strlen(line);
    return len == 0
        || line[0] == '|' 
        || line[len-1] == '|' 
        || strstr(line, "||");
}

// Separa la línea por '|' en commands[] y devuelve la cantidad
int split_pipes(char *line, char **commands) {
    int cnt = 0;
    for (char *tok = strtok(line, "|"); tok && cnt < MAX_CMDS; tok = strtok(NULL, "|")) {
        while (isspace((unsigned char)*tok)) tok++;
        commands[cnt++] = strdup(tok);
    }
    return cnt;
}

// Separa un comando en argv[], manejando comillas dobles
char **split_args(char *cmd) {
    char **argv = calloc(MAX_ARGS, sizeof(char*));
    int i = 0;
    while (*cmd) {
        while (isspace((unsigned char)*cmd)) cmd++;
        if (!*cmd) break;
        char *start = cmd;
        if (*cmd == '"') {
            start = ++cmd;
            while (*cmd && *cmd != '"') cmd++;
        } else {
            while (*cmd && !isspace((unsigned char)*cmd)) cmd++;
        }
        if (*cmd) *cmd++ = '\0';
        argv[i++] = strdup(start);
    }
    argv[i] = NULL;
    return argv;
}

// Ejecuta comandos conectados por pipes; args_list[i] es el argv para el comando i
void run_pipeline(char ***args_list, int cmd_count) {
    int in_fd = STDIN_FILENO, fd[2];
    for (int i = 0; i < cmd_count; ++i) {
        if (i < cmd_count - 1 && pipe(fd) < 0) { perror("pipe"); exit(1); }
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); exit(1); }
        if (pid == 0) {
            if (in_fd != STDIN_FILENO) dup2(in_fd, STDIN_FILENO), close(in_fd);
            if (i < cmd_count - 1) close(fd[0]), dup2(fd[1], STDOUT_FILENO), close(fd[1]);
            execvp(args_list[i][0], args_list[i]);
            fprintf(stderr, "%s: command not found\n", args_list[i][0]);
            exit(1);
        }
        if (in_fd != STDIN_FILENO) close(in_fd);
        if (i < cmd_count - 1) { close(fd[1]); in_fd = fd[0]; }
    }
    while (wait(NULL) > 0);
}

// Libera la memoria para commands[] y args_list[][]
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
