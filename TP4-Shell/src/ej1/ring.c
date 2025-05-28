#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void xread(int fd, void *buf, size_t size) {
    ssize_t r = read(fd, buf, size);
    if (r != (ssize_t)size) {
        perror("read");
        exit(1);
    }
}

void xwrite(int fd, void *buf, size_t size) {
    ssize_t w = write(fd, buf, size);
    if (w != (ssize_t)size) {
        perror("write");
        exit(1);
    }
}

void create_ring_pipes(int ring[][2], int n) {
    for (int i = 0; i < n; i++) {
        if (pipe(ring[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }
}

void close_unused_pipes(int ring[][2], int n, int me) {
    for (int i = 0; i < n; i++) {
        if (i != me) close(ring[i][1]);
        if (i != (me - 1 + n) % n) close(ring[i][0]);
    }
}

void child_process_logic(int me, int n, int start,
                         int ring[][2], int p2c[2], int c2p[2]) {
    close_unused_pipes(ring, n, me);

    if (me == start) {
        close(p2c[1]);
        close(c2p[0]);
    } else {
        close(p2c[0]); close(p2c[1]);
        close(c2p[0]); close(c2p[1]);
    }

    int val;
    int in_fd = (me == start) ? p2c[0] : ring[(me - 1 + n) % n][0];
    int out_fd = ring[me][1];

    if (me == start) {
        xread(in_fd, &val, sizeof(val));
        val++;
        xwrite(out_fd, &val, sizeof(val));
        xread(ring[(me - 1 + n) % n][0], &val, sizeof(val));
        xwrite(c2p[1], &val, sizeof(val));
    } else {
        xread(in_fd, &val, sizeof(val));
        val++;
        xwrite(out_fd, &val, sizeof(val));
    }

    exit(0);
}

void parent_process_logic(int n, int ring[][2], int p2c[2], int c2p[2], int initial_val) {
    for (int i = 0; i < n; i++) {
        close(ring[i][0]);
        close(ring[i][1]);
    }
    close(p2c[0]);
    close(c2p[1]);

    xwrite(p2c[1], &initial_val, sizeof(initial_val));
    xread(c2p[0], &initial_val, sizeof(initial_val));
    printf("Valor final recibido por el padre: %d\n", initial_val);
}

int main(int argc, char **argv)
{
    setbuf(stdout, NULL);  // stdout sin buffering

    if (argc != 4) {
        printf("Uso: anillo <n> <c> <s>\n");
        exit(1);
    }

    int n = atoi(argv[1]);
    int initial_val = atoi(argv[2]);
    int start = atoi(argv[3]);

    if (n < 3 || start < 0 || start >= n) {
        fprintf(stderr, "Error: n >= 3 y 0 <= s < n\n");
        exit(1);
    }

    printf("Se crearán %d procesos, se enviará el caracter %d desde proceso %d\n",
           n, initial_val, start);
    fflush(stdout);
    
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

    for (int i = 0; i < n; i++) wait(NULL);
    return 0;
}
