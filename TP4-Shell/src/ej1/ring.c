#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>

enum { READ = 0, WRITE = 1 };

void xread(int fd, void *buf, size_t size) {
    size_t total = 0;
    while (total < size) {
        ssize_t r = read(fd, (char*)buf + total, size - total);
        if (r <= 0) {
            perror("read");
            exit(1);
        }
        total += r;
    }
}

void xwrite(int fd, void *buf, size_t size) {
    size_t total = 0;
    while (total < size) {
        ssize_t w = write(fd, (char*)buf + total, size - total);
        if (w <= 0) {
            perror("write");
            exit(1);
        }
        total += w;
    }
}

void create_ring_pipes(int ring[][2], int n) {
    for (int i = 0; i < n; i++) {
        if (pipe(ring[i]) == -1) {
            fprintf(stderr, "Error creando pipe del anillo[%d]\n", i);
            perror("pipe");
            exit(1);
        }
    }
}

void close_unused_pipes(int ring[][2], int n, int me) {
    for (int i = 0; i < n; i++) {
        if (i != me) close(ring[i][WRITE]);
        if (i != (me - 1 + n) % n) close(ring[i][READ]);
    }
}

void child_process_logic(int me, int n, int start,
                         int ring[][2], int p2c[2], int c2p[2]) {
    close_unused_pipes(ring, n, me);

    if (me == start) {
        close(p2c[WRITE]);
        close(c2p[READ]);
    } else {
        close(p2c[READ]); close(p2c[WRITE]);
        close(c2p[READ]); close(c2p[WRITE]);
    }

    int val;
    int in_fd = (me == start) ? p2c[READ] : ring[(me - 1 + n) % n][READ];
    int out_fd = ring[me][WRITE];

    xread(in_fd, &val, sizeof(val));
    val++;
    xwrite(out_fd, &val, sizeof(val));

    if (me == start) {
        xread(ring[(me - 1 + n) % n][READ], &val, sizeof(val));
        xwrite(c2p[WRITE], &val, sizeof(val));
    }

    exit(0);
}

void parent_process_logic(int n, int ring[][2], int p2c[2], int c2p[2], int initial_val) {
    for (int i = 0; i < n; i++) {
        close(ring[i][READ]);
        close(ring[i][WRITE]);
    }
    close(p2c[READ]);
    close(c2p[WRITE]);

    xwrite(p2c[WRITE], &initial_val, sizeof(initial_val));
    close(p2c[WRITE]);  // muy importante para evitar bloqueos

    xread(c2p[READ], &initial_val, sizeof(initial_val));
    close(c2p[READ]);

    printf("Valor final recibido por el padre: %d\n", initial_val);
}

int main(int argc, char **argv)
{
    setbuf(stdout, NULL);  // stdout sin buffering

    if (argc != 4) {
        fprintf(stderr, "Uso: %s <n> <c> <s>\n", argv[0]);
        exit(1);
    }

    int n = atoi(argv[1]);
    int initial_val = atoi(argv[2]);
    int start = atoi(argv[3]);

    if (n < 3 || start < 0 || start >= n) {
        fprintf(stderr, "Error: n >= 3 y 0 <= s < n\n");
        exit(1);
    }

    // Validación de overflow
    if (initial_val > INT_MAX - n) {
        fprintf(stderr, "Error: desborde positivo (initial_val + n > INT_MAX)\n");
        exit(1);
    }
    if (initial_val < INT_MIN + n) {
        fprintf(stderr, "Error: desborde negativo (initial_val + n < INT_MIN)\n");
        exit(1);
    }

    printf("Se crearán %d procesos, se enviará el caracter %d desde proceso %d\n",
           n, initial_val, start);

    int ring[n][2], p2c[2], c2p[2];
    create_ring_pipes(ring, n);
    if (pipe(p2c) == -1 || pipe(c2p) == -1) {
        perror("pipe extra");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            child_process_logic(i, n, start, ring, p2c, c2p);
        }
    }

    parent_process_logic(n, ring, p2c, c2p, initial_val);

    for (int i = 0; i < n; i++) {
        int status;
        waitpid(-1, &status, 0);
    }

    return 0;
}

