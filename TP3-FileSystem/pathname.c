#include "pathname.h"
#include "directory.h"
#include <string.h>

/*
 * Encuentra el inodo asociado a una ruta absoluta.
 * Retorna el número de inodo, o -1 si la ruta no es válida.
 */
int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if (!pathname || pathname[0] != '/')
        return -1;

    int inumber = ROOT_INUMBER;
    const char *ptr = pathname + 1;
    char name[14];

    while (*ptr) {
        int len = 0;
        while (*ptr && *ptr != '/' && len < (int)sizeof(name) - 1)
            name[len++] = *ptr++;
        name[len] = '\0';

        if (*ptr == '/')
            ptr++;

        struct direntv6 entry;
        if (directory_findname(fs, name, inumber, &entry) < 0)
            return -1;

        inumber = entry.d_inumber;
    }

    return inumber;
}
