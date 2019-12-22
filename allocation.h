#ifndef KIV_ZOS_ALLOCATION_H
#define KIV_ZOS_ALLOCATION_H

/*
 * Hlavičky
 */
#include <stdint.h>
#include "inode.h"

/*
 * Konstanty
 */

/*
 * Struktury
 */

/**
 * Přidělí danému i-uzlu data bloky ve VFS, počet přidělených bloků je vypočten podle
 * potřebné velikosti v byte.
 *
 * @param filename soubor VFS
 * @param bytes počet byte k alokaci
 * @param inode_ptr inode k alokaci
 * @return počet alokovaných data bloků (return < 0 - chyba | 0 to INT_32_MAX)
 */
int32_t allocate_bytes(char *filename, int64_t bytes, struct inode *inode_ptr);

/**
 * Přidělí danému i-uzlu data bloky ve VFS, počet přidělených bloků je přímo předán
 * hodnotou funkce
 *
 * @param filename soubor VFS
 * @param allocation_size počet byte k alokaci
 * @param inode_ptr inode k alokaci
 * @return počet alokovaných data bloků (return < 0 - chyba | 0 to INT_32_MAX)
 */
int32_t allocate_data_blocks(char *filename, int32_t allocation_size, struct inode *inode_ptr);

/**
 * Nastaví data v clusteru, začínající adresou address na 0
 *
 * @param filename soubor vfs
 * @param address pořátek clusteru
 * @return výsledek operace (return < 0 chyba | 1 = OK)
 */
bool allocation_clear_cluster(char *filename, int32_t address);

/**
 * Dealokuje všechny alokované databloky v INODE
 *
 * @param filename soubor vfs
 * @param inode_ptr ukazatel na strukturu inode
 * @return výsledek operace
 */
int32_t deallocate(char *filename, struct inode *inode_ptr);

#endif //KIV_ZOS_ALLOCATION_H
