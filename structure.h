#ifndef KIV_ZOS_STRUCTURE_H
#define KIV_ZOS_STRUCTURE_H

/*
 * Vyžadované hlavičky
 */
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include "bool.h"
#include "superblock.h"
#include "debug.h"
#include "inode.h"
#include "parsing.h"

/*
 * Konstanty
 */
#define IMPL_NON_DATA_PERCENTAGE 4
#define IMPL_CLUSTER_SIZE 4096

#define VFS_FILE 0
#define VFS_DIRECTORY 1
#define VFS_SYMLINK 2

/*
 * Struktury
 */
// None


/**
 * Vyplní soubor znaky \0 do cílové velikosti
 *
 * @param vfs_file ukazatel na otevřený soubor
 * @param size cílová velikost systému
 */
void file_set_size(FILE *vfs_file, int32_t size);


/**
 * Na základě celkové velikosti FS vypočítá zbylé parametry VFS
 *
 *
 * @param superblock_ptr ukazatel na superblock
 * @return uspěch operace (0 | 1 )
 */
bool structure_calculate(struct superblock *superblock_ptr);

/**
 * Vytvoří soubor s daným názvem, který bude fyzickou reprezentací VFS
 *
 * @param vfs_filename název VFS souboru
 * @param superblock_ptr ukazatel na superblock
 * @return úspěch operace (0 | 1)
 */
bool vfs_create(char *vfs_filename, struct superblock *superblock_ptr);


#endif //KIV_ZOS_STRUCTURE_H
