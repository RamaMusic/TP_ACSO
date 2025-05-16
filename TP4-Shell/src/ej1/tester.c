#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

int test_count = 0;
int fail_count = 0;

void print_result(const char* desc, int passed) {
    test_count++;
    if (passed) {
        printf("  ✅ %s\n", desc);
    } else {
        printf("  ❌ %s\n", desc);
        fail_count++;
    }
}

void expect_write(int fd, void *buf, size_t size, const char* desc, int should_fail) {
    errno = 0;
    ssize_t r = write(fd, buf, size);

    int failed = (r == -1);
    int passed = (should_fail && failed) || (!should_fail && !failed && r == (ssize_t)size);

    print_result(desc, passed);

    if (failed) {
        fprintf(stderr, "    → Error en '%s': write() devolvió %zd, errno=%d (%s)\n",
                desc, r, errno, strerror(errno));
    }
}

#define EXPECT_FAIL(fd, buf, size, desc) expect_write(fd, buf, size, desc " ❌ [debe fallar]", 1)
#define EXPECT_SUCCESS(fd, buf, size, desc) expect_write(fd, buf, size, desc " ✅ [debe funcionar]", 0)

int main() {
    signal(SIGPIPE, SIG_IGN);  // Evitamos que nos mate SIGPIPE
    int val = 123;

    // 1. EBADF: escritura con pipe cerrado
    {
        int fd[2]; pipe(fd);
        close(fd[1]);
        EXPECT_FAIL(fd[1], &val, sizeof(val), "write en pipe con escritura cerrada (EBADF)");
        close(fd[0]);
    }

    // 2. EPIPE: pipe sin lector
    {
        int fd[2]; pipe(fd);
        close(fd[0]);
        EXPECT_FAIL(fd[1], &val, sizeof(val), "write en pipe sin lector (EPIPE)");
        close(fd[1]);
    }

    // 3. EBADF: descriptor inválido
    {
        EXPECT_FAIL(-1, &val, sizeof(val), "write en descriptor inválido (-1)");
    }

    // 4. EBADF: archivo solo lectura
    {
        int fd = open("tester.c", O_RDONLY);
        if (fd >= 0) {
            EXPECT_FAIL(fd, &val, sizeof(val), "write en archivo abierto como solo lectura");
            close(fd);
        } else {
            print_result("write en archivo readonly (no se pudo abrir el archivo)", 0);
        }
    }

    // 5. OK: escritura válida
    {
        int fd[2]; pipe(fd);
        EXPECT_SUCCESS(fd[1], &val, sizeof(val), "escritura válida en pipe");
        close(fd[0]); close(fd[1]);
    }

    printf("\nResumen: %d tests, %d fallaron\n", test_count, fail_count);
    return fail_count == 0 ? 0 : 1;
}
