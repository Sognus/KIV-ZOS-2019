#ifndef KIV_ZOS_VFS_IO_H
#define KIV_ZOS_VFS_IO_H

#include "bool.h"
#include "inode.h"
#include <stdio.h>

// Struktura pro uložení kontextu při práci se souborem uvnitř inode
typedef struct VFS_FILE {
    char *vfs_file;                 // Ukazatel na řetězec s cestou k datovému souboru VFS
    struct inode *inode_ptr;        // Ukazatel na inode, se kterou pracujeme
    int64_t offset;                 // Počet bytů od začátku souboru odkud čteme
} VFS_FILE;


/**
 * Nastaví offset pro strukturu VFS_FILE
 *
 * @param vfs_file ukazatel na strukturu VFS_FILE
 * @param offset vstupní hodnota nastavení
 * @param type typ nastavení (typy dodržují SEEK_* v stdio.h)
 * @return (return < 0: chyba | return >= 0: v pořádku)
 */
int32_t vfs_seek(VFS_FILE *vfs_file, int64_t offset, int type);

/**
 * Přečte daný počet struktur dané velikosti ze souboru vfs_file uloženého ve virtuálním FS
 *
 * @param destination ukazatel na místo uložení
 * @param read_item_size velikost čtených dat
 * @param read_item_count počet opakování při čtení dat
 * @param vfs_file ukazatel na soubor
 * @return počet přečtených byte
 */
size_t vfs_read(void *destination, size_t read_item_size, size_t read_item_count, VFS_FILE *vfs_file);

/**
 * Zapíše do souboru vfs_file (do virtuálního FS) danou velikost dat s daným opakováním
 *
 * @param source ukazatel odkud se data budou číst
 * @param write_item_size velikost zapisovaných dat
 * @param write_item_count počet opakování při zápisu
 * @param vfs_file ukazatel na soubor ve VFS
 * @return počet zapsaných byte
 */
size_t vfs_write(void *source, size_t write_item_size, size_t write_item_count, VFS_FILE *vfs_file);

/**
 * Vytvoří kontext pro práci souboru - vždycky lze provádět čtení i zápis zároveň
 *
 * @param vfs_file cesta k souboru vfs.dat (apod.)
 * @param vfs_path cesta souboru uvnitř virtuálního filesystému.
 * @return (VFS_FILE* | NULL)
 */
VFS_FILE *vfs_open(char *vfs_file, char *vfs_path);

/**
 * Vytvoří kontext pro práci souboru - vždycky lze provádět čtení i zápis zároveň
 *
 * @param vfs_file cesta k souboru vfs.dat (apod.)
 * @param vfs_path ID inode k otevření
 * @return
 */
VFS_FILE *vfs_open_inode(char *vfs_file, int32_t inode_id);

/**
 * Zavře virtuální soubor a uvolní pamět
 *
 * @param file virtuální soubor k zavření
 * @return indikace výsledku
 */
bool vfs_close(VFS_FILE *file);

#endif //KIV_ZOS_VFS_IO_H
