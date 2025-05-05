#include "file.h"
#include "inode.h"
#include "diskimg.h"
#include <stdint.h>

/* Lee un bloque lógico de un archivo y lo copia en el buffer.
   Devuelve: -1 en caso de error, 0 si blockNum excede el archivo,
   tamaño válido para último bloque parcial, o 512 para bloque completo. */
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    struct inode node;
    if (inode_iget(fs, inumber, &node) < 0 || blockNum < 0)
        return -1;

    int size = inode_getsize(&node);
    int total = (size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    if (blockNum >= total)
        return 0;

    int diskBlk = inode_indexlookup(fs, &node, blockNum);
    if (diskBlk < 0)
        return -1;

    if (diskimg_readsector(fs->dfd, diskBlk, buf) != DISKIMG_SECTOR_SIZE)
        return -1;

    if (blockNum == total - 1 && (size % DISKIMG_SECTOR_SIZE) != 0)
        return size % DISKIMG_SECTOR_SIZE;

    return DISKIMG_SECTOR_SIZE;
}
