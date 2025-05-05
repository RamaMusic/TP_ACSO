#include "inode.h"
#include "diskimg.h"
#include <stdint.h>

/*
 * inode_iget:
 *   Carga el inodo número 'inumber' del dispositivo de disco del filesystem 'fs'.
 *   Devuelve 0 si tuvo éxito, o -1 en caso de error.
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    if (inumber < 1 || inumber > fs->superblock.s_isize * 16)
        return -1;

    int sector = INODE_START_SECTOR + (inumber - 1) / 16;
    struct inode buffer[16];

    if (diskimg_readsector(fs->dfd, sector, buffer) != DISKIMG_SECTOR_SIZE)
        return -1;

    *inp = buffer[(inumber - 1) % 16];
    return 0;
}

/*
 * inode_indexlookup:
 *   Retorna el bloque físico en disco para un bloque lógico 'blockNum'
 *   del inodo 'inp'. Soporta bloques directos, indirectos simples y dobles.
 *   Devuelve el número de bloque (>0) o -1 si no existe o hay error.
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    if (!(inp->i_mode & IALLOC) || blockNum < 0)
        return -1;

    /* Bloques directos */
    if (!(inp->i_mode & ILARG)) {
        if (blockNum >= 8)
            return -1;
        int b = inp->i_addr[blockNum];
        return b ? b : -1;
    }

    const int PTRS = DISKIMG_SECTOR_SIZE / sizeof(uint16_t);

    /* Single indirect (primeros 7 sectores) */
    if (blockNum < 7 * PTRS) {
        int idx = blockNum / PTRS;
        int off = blockNum % PTRS;
        int sec = inp->i_addr[idx];
        if (!sec)
            return -1;

        uint16_t table[PTRS];
        if (diskimg_readsector(fs->dfd, sec, table) != DISKIMG_SECTOR_SIZE)
            return -1;
        int b = table[off];
        return b ? b : -1;
    }

    /* Double indirect */
    int dbl = inp->i_addr[7];
    if (!dbl)
        return -1;

    int rem = blockNum - 7 * PTRS;
    int i1  = rem / PTRS;
    int i2  = rem % PTRS;
    if (i1 >= PTRS || i2 >= PTRS)
        return -1;

    uint16_t lvl1[PTRS];
    if (diskimg_readsector(fs->dfd, dbl, lvl1) != DISKIMG_SECTOR_SIZE)
        return -1;

    int sec2 = lvl1[i1];
    if (!sec2)
        return -1;

    uint16_t lvl2[PTRS];
    if (diskimg_readsector(fs->dfd, sec2, lvl2) != DISKIMG_SECTOR_SIZE)
        return -1;
    int b = lvl2[i2];
    return b ? b : -1;
}

/*
 * inode_getsize:
 *   Devuelve el tamaño en bytes indicado por el inodo.
 */
int inode_getsize(struct inode *inp) {
    return (inp->i_size0 << 16) | inp->i_size1;
}
