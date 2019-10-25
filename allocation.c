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

/**
 * Nastaví data v clusteru, začínající adresou address na 0
 *
 * @param filename soubor vfs
 * @param address pořátek clusteru
 * @return výsledek operace (return < 0 chyba | 1 = OK)
 */
bool allocation_clear_cluster(char *filename, int32_t address){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        log_debug("allocation_clear_cluster: Nelze pouzit prazdne jmeno souboru!\n");
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        log_debug("alllocation_clear_cluster: Zadany soubor neexistuje!\n");
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        log_debug("allocation_clear_cluster: Nepodarilo se precist superblok!\n");
        return -3;
    }

    // Kontrola adresy - rozsah
    if(address < superblock_ptr->data_start_address || address > superblock_ptr->disk_size){
        log_debug("allocation_clear_cluster: Adresa k vymazani dat je mimo povoleny rozsah!\n");
        return -4;
    }

    int32_t cluster_index = inode_data_index_from_address(filename, address);

    // Kontrola adresy - cluster
    if(cluster_index < 0){
        log_debug("allocation_clear_cluster: Adresa k vymazanim neukazuje na pocatek datoveho bloku!\n");
        return -5;
    }

    FILE *file = fopen(filename, "r+b");

    if(file == NULL){
        log_debug("allocation_clear_cluster: Nepodarilo se otevrit soubor k prepisu\n");
        return -6;
    }

    fseek(file, address, SEEK_SET);
    char zero = 0;
    fwrite(&zero, sizeof(zero), superblock_ptr->cluster_size, file);
    fflush(file);
    fclose(file);

    log_trace("allocation_clear_cluster: Vynulovana data v clusteru %d\n", cluster_index);

    return TRUE;
}
