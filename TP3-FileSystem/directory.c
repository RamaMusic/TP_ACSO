#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <string.h>
#include <stdint.h>

/*
 * directory_findname:
 *   Recorre las entradas de un directorio y busca la que coincide con el nombre dado.
 *   Si la encuentra, copia la entrada (struct direntv6) en dirEnt.
 *   Retorna 0 en éxito, o -1 en caso de error o si no existe la entrada.
 */
int directory_findname(struct unixfilesystem *fs,
                       const char *name,
                       int dirinumber,
                       struct direntv6 *dirEnt)
{
    struct inode dirInode;
    
    if (inode_iget(fs, dirinumber, &dirInode) < 0)
        return -1;

    if (!(dirInode.i_mode & IALLOC) ||
        (dirInode.i_mode & IFMT) != IFDIR)
        return -1;

    int size       = inode_getsize(&dirInode);
    int blocks     = (size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    uint8_t buffer[DISKIMG_SECTOR_SIZE];

    for (int blk = 0; blk < blocks; ++blk) {
        int bytes = file_getblock(fs, dirinumber, blk, buffer);
        if (bytes < 0)
            return -1;

        int count = bytes / sizeof(struct direntv6);
        struct direntv6 *entries = (struct direntv6 *)buffer;

        for (int i = 0; i < count; ++i) {
            if (strncmp(entries[i].d_name, name,
                        sizeof(entries[i].d_name)) == 0) {
                *dirEnt = entries[i];
                return 0;
            }
        }
    }

    return -1;
}
