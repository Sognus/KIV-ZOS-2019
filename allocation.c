#include "allocation.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "debug.h"
#include "parsing.h"
#include "superblock.h"

/**
 *
 * Přidělí danému i-uzlu data bloky ve VFS, počet přidělených bloků je vypočten podle
 * potřebné velikosti v byte.
 *
 * @param filename soubor VFS
 * @param bytes počet byte k alokaci
 * @param inode_ptr inode k alokaci
 * @return počet alokovaných data bloků (return < 0 - chyba | 0 to INT_32_MAX)
 */
int32_t allocate_bytes(char *filename, int64_t bytes, struct inode *inode_ptr){
    // Kontrola ukazatele
    if(inode_ptr == NULL){
        log_debug("allocate_bytes: Nelze alokovat misto pro inodu NULL!\n");
        return -1;
    }

    // Kontrola počtu byte
    if(bytes < 1){
        log_debug("allocate_bytes: Nelze alokovat 0 a méně byte!\n");
        return -2;
    }

    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        log_debug("allocate_bytes: Nelze pouzit prazdne jmeno souboru!\n");
        return -3;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        log_debug("allocate_bytes: Zadany soubor neexistuje!\n");
        return -4;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        log_debug("allocate_bytes: Nepodarilo se precist superblok!\n");
        return -5;
    }

    int32_t cluster_size = superblock_ptr->cluster_size;
    int32_t clusters_needed = (int32_t)(ceil((double)(bytes)/(double)(cluster_size))) ;
    free(superblock_ptr);

    return allocate_data_blocks(filename, clusters_needed, inode_ptr);
}

/**
 * TODO: implement
 *
 * Přidělí danému i-uzlu data bloky ve VFS, počet přidělených bloků je přímo předán
 * hodnotou funkce
 *
 * @param filename soubor VFS
 * @param allocation_size počet byte k alokaci
 * @param inode_ptr inode k alokaci
 * @return počet alokovaných data bloků (return < 0 - chyba | 0 to INT_32_MAX)
 */
int32_t allocate_data_blocks(char *filename, int32_t allocation_size, struct inode *inode_ptr){
    // Kontrola ukazatele
    if(inode_ptr == NULL){
        log_debug("allocate_data_blocks: Nelze alokovat misto pro inodu NULL!\n");
        return -1;
    }

    // Kontrola počtu byte
    if(allocation_size < 1){
        log_debug("allocate_data_blocks: Nelze alokovat 0 a data bloku!\n");
        return -2;
    }

    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        log_debug("allocate_data_blocks: Nelze pouzit prazdne jmeno souboru!\n");
        return -3;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        log_debug("allocate_data_blocks: Zadany soubor neexistuje!\n");
        return -4;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        log_debug("allocate_data_blocks: Nepodarilo se precist superblok!\n");
        return -5;
    }

    // TODO: Ziskavani volnych bloku v cyklu dokud je potřeba ziskavat
    // TODO: Zápis získaných bloků do inode
    // TODO: přepis změn inode do VFS
    return 0;
}
