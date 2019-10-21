#ifndef KIV_ZOS_INODE_H
#define KIV_ZOS_INODE_H

/*
 * Nutné hlavičky
 */
#include <stdint.h>
#include "bool.h"


/*
 * Konstanty
 */
#define ID_ITEM_FREE 0

/*
 * Struktury
 */
struct inode {
    int32_t id;                // ID i-uzlu; pokud ID == ID_ITEM_FREE, je položka volná
    int8_t type;               // Typ i-uzlu; 0 = soubor; 1 = složka; 2 = symlink
    int8_t references;         // Počet odkazů na i-uzel; používá se pro hardlinky
    int32_t file_size;         // Velikost souboru v bytech
    int32_t direct1;           // 1. přímý odkaz na datové bloky
    int32_t direct2;           // 2. přímý odkaz na datové bloky
    int32_t direct3;           // 3. přímý odkaz na datové bloky
    int32_t direct4;           // 4. přímý odkaz na datové bloky
    int32_t direct5;           // 5. přímý odkaz na datové bloky
    int32_t indirect1;         // 1. nepřímý odkaz (odkaz - datové bloky)
    int32_t indirect2;         // 2. nepřímý odkaz (odkaz - odkaz - datové bloky)
};

/**
 * Vypíše obsah struktury inode
 *
 * @param ptr ukazatel na strukturu inode
 */
void inode_print(struct inode *ptr);

#endif //KIV_ZOS_INODE_H
