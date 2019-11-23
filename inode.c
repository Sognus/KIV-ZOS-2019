#include "inode.h"
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "superblock.h"
#include "parsing.h"
#include "bitmap.h"
#include "allocation.h"
#include <math.h>

/**
 * Vypíše obsah struktury inode
 *
 * @param ptr ukazatel na strukturu inode
 */
void inode_print(struct inode *ptr){
    if (ptr == NULL) {
        log_trace("inode_print: Ukazatel na strukturu inode je NULL!\n");
        return;
    }

    log_info("*** INODE\n");
    log_info("ID: %d\n", ptr->id);
    log_info("Type: %d\n", ptr->type);
    log_info("References: %d\n", ptr->references);
    log_info("File size: %d\n", ptr->file_size);
    log_info("Pointer count: %d\n", ptr->allocated_clusters);
    log_info("Direct pointer 1: %ď\n", ptr->direct1);
    log_info("Direct pointer 2: %ď\n", ptr->direct2);
    log_info("Direct pointer 3: %ď\n", ptr->direct3);
    log_info("Direct pointer 4: %ď\n", ptr->direct4);
    log_info("Direct pointer 5: %ď\n", ptr->direct5);
    log_info("Single indirect pointer: %d\n", ptr->indirect1);
    log_info("Double indirect pointer: %d\n", ptr->indirect2);
    log_info("*** INODE END\n");
}

/**
 * Zapíše obsah struktury inode na adresu ve VFS určenou indexem
 *
 * @param filename soubor vfs
 * @param inode_index index inode ve VFS
 * @param inode_ptr struktura k zapsání
 * @return výsledek operace
 */
int32_t inode_write_to_index(char *filename, int32_t inode_index, struct inode *inode_ptr){
    // Nalezení adresy podle indexu
    int32_t inode_address = inode_index_to_adress(filename, inode_index);

    // Ověření existence adresy
    if(inode_address < 0){
        return inode_address;
    }

    // Zápis na adresu
    return inode_write_to_address(filename, inode_address, inode_ptr);
}

/**
 * Zapíše obsah struktury inode na adresu ve VFS
 *
 * @param filename soubor vfs
 * @param inode_address adresa inode ve VFS
 * @param inode_ptr struktura k zapsání
 * @return výsledek operace
 */
int32_t inode_write_to_address(char *filename, int32_t inode_address, struct inode *inode_ptr){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        return -3;
    }

    // Ověření adresy k zápisu
    if(inode_address > superblock_ptr->data_start_address - sizeof(struct inode)){
        free(superblock_ptr);
        return -4;
    }

    // Otevření souboru pro čtení
    FILE *file = fopen(filename, "r+b");

    // Ověření otevření souboru
    if(file == NULL){
        free(superblock_ptr);
        return -5;
    }


    // Nastavení ukazatele zápisu
    fseek(file, inode_address, SEEK_SET);
    fflush(file);
    fwrite(inode_ptr, sizeof(struct inode), 1, file);
    fclose(file);

    // Akce se podařila
    free(superblock_ptr);
    return TRUE;
}


/**
 * Na základě indexu vrátí adresu inode
 *
 * @param filename soubor vfs
 * @param inode_index index inode
 * @return
 */
int32_t inode_index_to_adress(char *filename, int32_t inode_index){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        return -3;
    }

    int inode_size = sizeof(struct inode);
    int inode_adress_start = superblock_ptr->inode_start_address;
    free(superblock_ptr);
    return inode_adress_start + (inode_index * inode_size);
}

/**
 * Vrátí první volný index pro inode
 *
 * @param filename
 * @return
 */
int32_t inode_find_free_index(char *filename){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        return -3;
    }

    // Otevření souboru pro čtení
    FILE *file = fopen(filename, "r");

    // Ověření otevření souboru
    if(file == NULL){
        free(superblock_ptr);
        return -4;
    }

    int32_t inode_start = superblock_ptr->inode_start_address;
    fseek(file, inode_start ,SEEK_SET);
    struct inode *inode_ptr = malloc(sizeof(struct inode));
    int index = 0;

    // Lineární čtení dat, kde jsou uložené inode
    while(ftell(file) < superblock_ptr->data_start_address){
        fread(inode_ptr, sizeof(struct inode), 1, file);


        // Pokud jsme přečetli všechny inode, nenašli jsme žádný volný
        if(ftell(file) >= superblock_ptr->data_start_address){
            return -5;
        }

        if(inode_ptr->id == ID_ITEM_FREE){
            break;
        }

        index++;
    }

    // Uvolnění superbloku
    free(superblock_ptr);
    free(inode_ptr);
    fclose(file);

    // Návrat indexu
    return index;

}

/**
 * Pokusí se o přečtení struktury inode z VFS a vrátí ukazatel
 * hledá inode dle indexu
 *
 * @param filename soubor VFS
 * @param inode_index index inode
 * @return výsledek operace (PTR | NULL)
 */
struct inode *inode_read_by_index(char *filename, int32_t inode_index){
    // Nalezení adresy podle indexu
    int32_t inode_address = inode_index_to_adress(filename, inode_index);

    // Ověření existence adresy
    if(inode_address < 0){
        return NULL;
    }

    return inode_read_by_address(filename, inode_address);
}


/**
 * Pokusí se o přečtení struktury inode z VFS a vrátí ukazatel
 * hledá inode dle indexu
 *
 * @param filename soubor VFS
 * @param inode_address adresa inode ve VFS
 * @return výsledek operace (PTR | NULL)
 */
struct inode *inode_read_by_address(char *filename, int32_t inode_address){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        return NULL;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        return NULL;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        return NULL;
    }

    // Ověření adresy ke čtení
    if(inode_address > superblock_ptr->data_start_address - sizeof(struct inode)){
        free(superblock_ptr);
        return NULL;
    }

    // Otevření souboru pro čtení
    FILE *file = fopen(filename, "rb");

    // Ověření otevření souboru
    if(file == NULL){
        free(superblock_ptr);
        return NULL;
    }

    struct inode *inode_ptr = malloc(sizeof(struct inode));
    fseek(file, inode_address, SEEK_SET);
    fread(inode_ptr, sizeof(struct inode), 1, file);
    fclose(file);

    //Inode je prázdná
    if(inode_ptr->id == 0){
        free(superblock_ptr);
        free(inode_ptr);
        return NULL;
    }

    free(superblock_ptr);
    return inode_ptr;
}

/**
 *
 * Kontrolní funkce, která ověří, zda lze převést adresu na index data bloku
 *
 * @param filename soubor vfs
 * @param address adresa ve VFS
 * @return výsledek operace (return < 0 - chyba | return >= 0 - validní index)
 */
int32_t inode_data_index_from_address(char *filename, int32_t address){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        return -3;
    }

    int32_t current_address = superblock_ptr->data_start_address;
    int32_t cluster_size = superblock_ptr->cluster_size;
    int32_t index = 0;

    while(current_address < superblock_ptr->disk_size){
        if(current_address == address){
            break;
        }

        current_address = current_address + cluster_size;
        index++;
    }

    if(index > superblock_ptr->cluster_count){
        free(superblock_ptr);
        return -4;
    }

    free(superblock_ptr);
    return index;
}

/**
 * Přidá nový ukazatel na datový blok pro strukturu
 *
 * @deprecated Prochází lineárně všechny existující odkazy a hledá volný odkaz - pomalé
 *
 * @param filename soubor VFS
 * @param inode_ptr ukazatel na pozměňovaný inode
 * @return výsledek operace
 */
bool inode_add_data_address_slow(char *filename, struct inode *inode_ptr, int32_t address){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        return -3;
    }

    // Kontrola zda adresa je validní adresou začátku clusteru
    if(inode_data_index_from_address(filename, address) < 0){
        return -4;
    }

    bool address_writen = FALSE;
    int32_t index_written = -50;

    /*
     * Kontrola možnosti zapsání přímých adres
     */

    if(inode_ptr->direct1 == 0 && address_writen == FALSE){
        inode_ptr->direct1 = address;
        index_written = 1;
        address_writen = TRUE;
        log_trace("inode_add_data_address_slow: Adresa databloku ulozena do direct1 (index 1)\n");
    }

    if(inode_ptr->direct2 == 0 && address_writen == FALSE){
        inode_ptr->direct2 = address;
        index_written = 2;
        address_writen = TRUE;
        log_trace("inode_add_data_address_slow: Adresa databloku ulozena do direct2 (index 2)\n");
    }

    if(inode_ptr->direct3 == 0 && address_writen == FALSE){
        inode_ptr->direct3 = address;
        index_written = 3;
        address_writen = TRUE;
        log_trace("inode_add_data_address_slow: Adresa databloku ulozena do direct3 (index 3)\n");
    }

    if(inode_ptr->direct4 == 0 && address_writen == FALSE){
        inode_ptr->direct4 = address;
        index_written = 4;
        address_writen = TRUE;
        log_trace("inode_add_data_address_slow: Adresa databloku ulozena do direct4 (index 4)\n");
    }

    if(inode_ptr->direct5 == 0 && address_writen == FALSE){
        inode_ptr->direct5 = address;
        index_written = 5;
        address_writen = TRUE;
        log_trace("inode_add_data_address_slow: Adresa databloku ulozena do direct5 (index 5)\n");
    }

    /*
     * 1. nepřímý odkaz na clustery
     */

    // Alokace pro nepřímý odkaz v případě, že je potřeba
    if(inode_ptr->indirect1 == 0 && address_writen == FALSE){
        int32_t indirect1_allocation_index = bitmap_find_free_cluster_index(filename);

        if(indirect1_allocation_index < 0){
            log_debug("inode_add_data_address_slow: Nelze alokovat 1. neprimou adresu, nedostatek volnych clusteru!\n");
            return -5;
        }

        int32_t indirect1_allocation_address = bitmap_index_to_cluster_address(filename, indirect1_allocation_index);
        bitmap_set(filename, indirect1_allocation_index, 1, TRUE);

        // Nulování datového bloku
        allocation_clear_cluster(filename, indirect1_allocation_address);
        // Zápis do inode
        inode_ptr->indirect1 = indirect1_allocation_address;
        // Zápis na VFS
        inode_write_to_index(filename, inode_ptr->id - 1, inode_ptr);
        log_trace("inode_add_data_address_slow: Hodnota nepřímého odkazu pro ID=%d nastavena na %d\n", inode_ptr->id, inode_ptr->indirect1);
    }

    if(address_writen == FALSE) {
        index_written = 6;

        // Procházení 1. nepřímého odkazu a zápis
        int32_t *indirect1_iter_data = malloc(sizeof(int32_t));
        int32_t indirect1_iter_current = inode_ptr->indirect1;
        int32_t indirect1_iter_end = inode_ptr->indirect1 + superblock_ptr->cluster_size;

        FILE *file_read = fopen(filename, "r+b");

        while (indirect1_iter_current < indirect1_iter_end) {

            fseek(file_read, indirect1_iter_current, SEEK_SET);
            memset(indirect1_iter_data, 0, sizeof(int32_t));
            fread(indirect1_iter_data, sizeof(int32_t), 1, file_read);

            // Našli jsme místo kam zapsat
            if (*indirect1_iter_data == 0) {
                fseek(file_read, indirect1_iter_current, SEEK_SET);
                fwrite(&address, sizeof(int32_t), 1, file_read);
                fflush(file_read);

                address_writen = TRUE;
                log_trace("inode_add_data_address_slow: Adresa databloku ulozena do indirect1 (index %d)\n", index_written);
                break;
            }

            // Posun na další adresu
            indirect1_iter_current += sizeof(int32_t);
            index_written++;
        }

        fclose(file_read);
        free(indirect1_iter_data);
    }

    // Alokace pro nepřímý odkaz
    if(address_writen == FALSE) {

        // Alokace pro inode->indirect2 pokud je ukazatel NULL
        if (inode_ptr->indirect2 == 0) {
            int32_t indirect2_allocation_index = bitmap_find_free_cluster_index(filename);

            if (indirect2_allocation_index < 0) {
                log_debug("inode_add_data_address_slow: Nelze alokovat 2. neprimou adresu, nedostatek volnych clusteru!\n");
                return -6;
            }

            int32_t indirect2_allocation_address = bitmap_index_to_cluster_address(filename,
                                                                                   indirect2_allocation_index);
            bitmap_set(filename, indirect2_allocation_index, 1, TRUE);

            // Nulování datového bloku
            allocation_clear_cluster(filename, indirect2_allocation_address);
            // Zápis do inode
            inode_ptr->indirect2 = indirect2_allocation_address;
            // Zápis na VFS
            inode_write_to_index(filename, inode_ptr->id - 1, inode_ptr);
            log_trace("inode_add_data_address_slow: Hodnota 2. nepřímého odkazu pro ID=%d nastavena na %d\n", inode_ptr->id,
                      inode_ptr->indirect2);
        }

        // Nepřímá adresa typu 2 -> nepřímá adresa typu 1
        int32_t *indirect2_level1_iter_data = malloc(sizeof(int32_t));
        int32_t indirect2_level1_iter_current = inode_ptr->indirect2;
        int32_t indirect2_level1_iter_end = inode_ptr->indirect2 + superblock_ptr->cluster_size;

        FILE *indirect2_file_read = fopen(filename, "r+b");
        int32_t level1_debug_iter = 0;

        // Nepřímá adresa - iterace level 1
        while(indirect2_level1_iter_current < indirect2_level1_iter_end && address_writen == FALSE){
            memset(indirect2_level1_iter_data, 0, sizeof(int32_t));
            fseek(indirect2_file_read, indirect2_level1_iter_current, SEEK_SET);
            fread(indirect2_level1_iter_data, sizeof(int32_t), 1, indirect2_file_read);

            // Pokud je ukazatel NULL, alokuj nový cluster a vrat na něj adresu
            if(*indirect2_level1_iter_data == 0){
                int32_t indirect2_level1_allocation_index = bitmap_find_free_cluster_index(filename);

                if (indirect2_level1_allocation_index < 0) {
                    log_debug("inode_add_data_address_slow: Nelze alokovat 2. neprimou adresu úrovně 1, nedostatek volnych clusteru!\n");
                    return -6;
                }

                int32_t indirect2_level1_allocation_address = bitmap_index_to_cluster_address(filename,
                                                                                       indirect2_level1_allocation_index);
                bitmap_set(filename, indirect2_level1_allocation_index, 1, TRUE);

                // Nulování datového bloku
                allocation_clear_cluster(filename, indirect2_level1_allocation_address);

                // Zápis do inode
                fseek(indirect2_file_read, indirect2_level1_iter_current, SEEK_SET);
                fwrite(&indirect2_level1_allocation_address, sizeof(indirect2_level1_allocation_address), 1, indirect2_file_read);
                fflush(indirect2_file_read);

                // Zápis na VFS
                inode_write_to_index(filename, inode_ptr->id - 1, inode_ptr);
                log_trace("inode_add_data_address_slow: Hodnota 2. nepřímého odkazu level 1, iterace %d pro inode ID=%d nastavena na %d\n", level1_debug_iter,inode_ptr->id,
                          indirect2_level1_allocation_address);
            }

            // Nepřímá adresa - iterace level 2
            int32_t *indirect2_level2_iter_data = malloc(sizeof(int32_t));
            int32_t indirect2_level2_iter_current = *indirect2_level1_iter_data;
            int32_t indirect2_level2_iter_end = indirect2_level2_iter_current + superblock_ptr->cluster_size;

            int32_t level2_debug_iter = 0;

            while(indirect2_level2_iter_current < indirect2_level2_iter_end && address_writen == FALSE){
                // Prečtení adresy levelu 2
                memset(indirect2_level2_iter_data, 0, sizeof(int32_t));
                fseek(indirect2_file_read, indirect2_level2_iter_current, SEEK_SET);
                fread(indirect2_level2_iter_data, sizeof(int32_t), 1, indirect2_file_read);

                // Pokud je adresa NULL - alokuj a zapiš
                if(*indirect2_level2_iter_data == 0){
                    int32_t indirect2_level2_allocation_index = bitmap_find_free_cluster_index(filename);

                    if (indirect2_level2_allocation_index < 0) {
                        log_debug("inode_add_data_address_slow: Nelze alokovat 2. neprimou adresu úrovně 1, nedostatek volnych clusteru!\n");
                        return -6;
                    }

                    int32_t indirect2_level2_allocation_address = bitmap_index_to_cluster_address(filename,
                                                                                                  indirect2_level2_allocation_index);
                    bitmap_set(filename, indirect2_level2_allocation_index, 1, TRUE);

                    // Nulování datového bloku
                    allocation_clear_cluster(filename, indirect2_level2_allocation_address);

                    // Zápis do inode
                    fseek(indirect2_file_read, indirect2_level2_iter_current, SEEK_SET);
                    fwrite(&indirect2_level2_allocation_address, sizeof(indirect2_level2_allocation_address), 1, indirect2_file_read);
                    fflush(indirect2_file_read);

                    // Zápis na VFS
                    inode_write_to_index(filename, inode_ptr->id - 1, inode_ptr);
                    log_trace("inode_add_data_address_slow: Hodnota 2. nepřímého odkazu level 2, iterace %d pro inode ID=%d nastavena na %d\n", level2_debug_iter,inode_ptr->id,
                              indirect2_level2_allocation_address);

                    // Označení zápisu
                    address_writen = TRUE;
                    log_trace("inode_add_data_address_slow: Adresa databloku ulozena do indirect2 [level1=%d, level2=%d] (index %d)\n", level1_debug_iter, level2_debug_iter,index_written);
                }

                // Posun na další adresu v levelu 2
                indirect2_level2_iter_current += sizeof(int32_t);
                level2_debug_iter++;
                index_written++;
            }


            // Posun na další položku levelu 2
            indirect2_level1_iter_current += sizeof(int32_t);
            level1_debug_iter++;
            free(indirect2_level2_iter_data);
        }

        free(indirect2_level1_iter_data);
        fclose(indirect2_file_read);
    }


    // Zabrání data bloku v bitmapě
    if(address_writen == TRUE){
        int32_t data_index_claimed = inode_data_index_from_address(filename, address);
        bitmap_set(filename, data_index_claimed, 1, TRUE);
    }
    else{
        return -7;
    }

    // Návrat indexu odkazu v INODE na data blok
    // 1-5 = direct
    // 6-1030 = indirect1
    // zbytek = indirect2 - 1024 per level
    /*
     * INDIRECT2
     *      I2L1 - 1024
     *      I2L2 - 1024
     */
    return address_writen;
}

/**
 * Získá adresu uloženou na daném indexu uložených databloků
 *
 * @param filename soubor VFS
 * @param inode_ptr struktura inode
 * @param index index odkazu v inode
 * @return (return <= 0: chyba | return > 0: adresa databloku ve VFS)
 */
int32_t inode_get_datablock_index_value(char *filename, struct inode *inode_ptr, int32_t index){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        return -3;
    }

    // Kontrola ukazatele na inode
    if(inode_ptr == NULL) {
        free(superblock_ptr);
        return -4;
    }

    // Pokud se pokusíme přistoupit k indexu, který není alokován -> chyba
    if(index > inode_ptr->allocated_clusters){
        free(superblock_ptr);
        return -5;
    }

    // Přímé ukazatele 0-4
    if(index == 0){
        free(superblock_ptr);
        log_trace("inode_get_datablock_index_value: 0 -> direct1 -> %d\n", inode_ptr->direct1);
        return inode_ptr->direct1;
    }

    if(index == 1){
        free(superblock_ptr);
        log_trace("inode_get_datablock_index_value: 1 -> direct2 -> %d\n", inode_ptr->direct2);
        return inode_ptr->direct2;
    }

    if(index == 2){
        free(superblock_ptr);
        log_trace("inode_get_datablock_index_value: 2 -> direct3 -> %d\n", inode_ptr->direct3);
        return inode_ptr->direct3;
    }

    if(index == 3){
        free(superblock_ptr);
        log_trace("inode_get_datablock_index_value: 3 -> direct4 -> %d\n", inode_ptr->direct4);
        return inode_ptr->direct4;
    }

    if(index == 4){
        free(superblock_ptr);
        log_trace("inode_get_datablock_index_value: 4 -> direct5 -> %d\n", inode_ptr->direct5);
        return inode_ptr->direct5;
    }

    // 1. Nepřímý ukazatel: 5-1028
    if(index > 4 && index < 1029) {
        int32_t indirect1_index = index - 5;
        int32_t indirect1_address = inode_ptr->indirect1 + (indirect1_index * sizeof(int32_t));

        // Alokace paměti pro čtení
        int32_t *indirect_address_read = malloc(sizeof(int32_t));
        // Nulování obsahu paměti
        (*indirect_address_read) = 0;
        // Otevření souboru ke čtení
        FILE *file = fopen(filename, "r+b");
        // Nastavení ukazatele
        fseek(file, indirect1_address, SEEK_SET);
        // Přečtení data
        fread(indirect_address_read, sizeof(int32_t), 1, file);

        // Přesun dat na heap
        int32_t data_rtn = (*indirect_address_read);
        // Uvolnění zdrojů
        fclose(file);
        free(indirect_address_read);

        // Návrat hodnoty
        log_trace("inode_get_datablock_index_value: %d -> indirect1[%d] -> %d\n", index, indirect1_index, inode_ptr->direct1);
        free(superblock_ptr);
        return data_rtn;
    }

    if(index > 1028){
        int32_t indirect2_level1_index = (int32_t)floor(((double)(index-1029))/1024);
        int32_t indirect2_level2_index = (index - 1029) % 1024;

        // Otevření souboru ke čtení a zápisu
        FILE *indirect2_file = fopen(filename, "r+b");

        // Získání adresy ukazatele na datablok - úroven 1
        int32_t indirect2_level1_address = inode_ptr->indirect2 + (indirect2_level1_index * sizeof(int32_t));

        // Alokace paměti pro čtení
        int32_t *indirect2_level1_data = malloc(sizeof(int32_t));
        int32_t *indirect2_level2_data = malloc(sizeof(int32_t));

        // Nulování paměti
        memset(indirect2_level1_data, 0, sizeof(int32_t));
        // Nastavení místa čtení v souboru
        fseek(indirect2_file, indirect2_level1_address, SEEK_SET);
        // Čtení dat
        fread(indirect2_level1_data, sizeof(int32_t), 1, indirect2_file);

        // Čtení adresy level2
        if(*indirect2_level1_data != 0){
            // Adresa kam zapsat
            int32_t indirect2_level2_address = *indirect2_level1_data + (indirect2_level2_index * sizeof(int32_t));

            // Zápis adresy na level 2
            fseek(indirect2_file, indirect2_level2_address, SEEK_SET);
            fread(indirect2_level2_data, sizeof(int32_t), 1, indirect2_file);
        }

        // Uvolnění zdrojů a přesun na heap
        free(indirect2_level1_data);
        int32_t rtn_data = (*indirect2_level2_data);

        // Logging
        log_trace("inode_get_datablock_index_value: %d -> indirect2[%d][%d] -> %d\n", index, indirect2_level1_index, indirect2_level2_index, rtn_data);

        free(indirect2_level2_data);
        free(superblock_ptr);
        return rtn_data;

    }

    free(superblock_ptr);
    return 0;



}


/**
 * Přidá nový ukazatel na datový blok pro strukturu - rychlejší verze
 *
 * @param filename soubor VFS
 * @param inode_ptr ukazatel na pozměňovaný inode
 * @return výsledek operace
 */
bool inode_add_data_address(char *filename, struct inode *inode_ptr, int32_t address){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        return -3;
    }

    // Kontrola zda adresa je validní adresou začátku clusteru
    if(inode_data_index_from_address(filename, address) < 0){
        free(superblock_ptr);
        return -4;
    }

    // Kontrola ukazatele na inode
    if(inode_ptr == NULL) {
        free(superblock_ptr);
        return -5;
    }

    // Zda je adresa zapsaná
    bool address_writen = FALSE;

    // Zápis pro direct1 - direct5
    if(inode_ptr->allocated_clusters == 0){
        inode_ptr->direct1 = address;
        address_writen = TRUE;
        log_trace("inode_add_data_address: Adresa databloku ulozena do direct1 (pointer_index: 0)\n");
    }

    if(address_writen == FALSE && inode_ptr->allocated_clusters == 1){
        inode_ptr->direct2 = address;
        address_writen = TRUE;
        log_trace("inode_add_data_address: Adresa databloku ulozena do direct2 (pointer_index:  %d)\n", inode_ptr->allocated_clusters);
    }

    if(address_writen == FALSE && inode_ptr->allocated_clusters == 2){
        inode_ptr->direct3 = address;
        address_writen = TRUE;
        log_trace("inode_add_data_address: Adresa databloku ulozena do direct3 (pointer_index:  %d)\n", inode_ptr->allocated_clusters);
    }

    if(address_writen == FALSE && inode_ptr->allocated_clusters == 3){
        inode_ptr->direct4 = address;
        address_writen = TRUE;
        log_trace("inode_add_data_address: Adresa databloku ulozena do direct4 (pointer_index:  %d)\n", inode_ptr->allocated_clusters);
    }

    if(address_writen == FALSE && inode_ptr->allocated_clusters == 4){
        inode_ptr->direct5 = address;
        address_writen = TRUE;
        log_trace("inode_add_data_address: Adresa databloku ulozena do direct5 (pointer_index:  %d)\n", inode_ptr->allocated_clusters);
    }

    // Alokace pro 1. nepřímý odkaz v případě, že je potřeba
    if(address_writen == FALSE && inode_ptr->allocated_clusters > 4 && inode_ptr->allocated_clusters < 1029 && inode_ptr->indirect1 == 0){
        int32_t indirect1_allocation_index = bitmap_find_free_cluster_index(filename);

        if(indirect1_allocation_index < 0){
            log_debug("inode_add_data_address: Nelze alokovat 1. neprimou adresu, nedostatek volnych clusteru!\n");
            free(superblock_ptr);
            return -5;
        }

        int32_t indirect1_allocation_address = bitmap_index_to_cluster_address(filename, indirect1_allocation_index);
        bitmap_set(filename, indirect1_allocation_index, 1, TRUE);

        // Nulování datového bloku
        allocation_clear_cluster(filename, indirect1_allocation_address);
        // Zápis do inode
        inode_ptr->indirect1 = indirect1_allocation_address;
        // Zápis na VFS
        inode_write_to_index(filename, inode_ptr->id - 1, inode_ptr);
        log_trace("inode_add_data_address: Hodnota nepřímého odkazu pro ID=%d nastavena na %d\n", inode_ptr->id, inode_ptr->indirect1);
    }

    // Zápis do indexů 5-1028
    if(address_writen == FALSE && inode_ptr->allocated_clusters > 4 && inode_ptr->allocated_clusters < 1029){
        // -5 = OFFSET PRO LINEÁRNÍ POSUN POČÁTKU INDEXACE -> vytvoří 0-1023
        int32_t indirect1_write_index = inode_ptr->allocated_clusters - 5;
        int32_t indirect1_write_address = inode_ptr->indirect1 + (indirect1_write_index * sizeof(int32_t));

        //log_trace("inode_add_data_address: Indirect2 Transformation %d->indirect2[%d]\n", inode_ptr->allocated_clusters, indirect1_write_index);

        FILE *indirect1_file_write = fopen(filename, "r+b");
        fseek(indirect1_file_write, indirect1_write_address, SEEK_SET);
        fwrite(&address, sizeof(address), 1, indirect1_file_write);
        fflush(indirect1_file_write);
        fclose(indirect1_file_write);

        log_trace("inode_add_data_address: Adresa databloku ulozena do indirect1[%d] (addr: %d, value: %d, pointer_index: %d)\n", indirect1_write_index, indirect1_write_address, address, inode_ptr->allocated_clusters);
        address_writen = TRUE;

    }

    // Alokace pro inode->indirect2 pokud je ukazatel NULL
    if (address_writen == FALSE && inode_ptr->allocated_clusters > 1028 && inode_ptr->indirect2 == 0) {
        int32_t indirect2_allocation_index = bitmap_find_free_cluster_index(filename);

        if (indirect2_allocation_index < 0) {
            log_debug("inode_add_data_address: Nelze alokovat 2. neprimou adresu, nedostatek volnych clusteru!\n");
            free(superblock_ptr);
            return -6;
        }

        int32_t indirect2_allocation_address = bitmap_index_to_cluster_address(filename,
                                                                               indirect2_allocation_index);
        bitmap_set(filename, indirect2_allocation_index, 1, TRUE);

        // Zápis do inode
        inode_ptr->indirect2 = indirect2_allocation_address;
        // Nulování datového bloku
        allocation_clear_cluster(filename, inode_ptr->indirect2);
        // Zápis na VFS
        inode_write_to_index(filename, inode_ptr->id - 1, inode_ptr);
        log_trace("inode_add_data_address: Hodnota 2. nepřímého odkazu pro ID=%d nastavena na %d\n", inode_ptr->id,
                  inode_ptr->indirect2);
    }

    // Zápis databloků pro indirect2
    if(address_writen == FALSE && inode_ptr->allocated_clusters > 1028 && inode_ptr->indirect2 != 0){
        int32_t indirect2_level1_write_index = (int32_t)floor(((double)(inode_ptr->allocated_clusters-1029))/1024);
        int32_t indirect2_level2_write_index = (inode_ptr->allocated_clusters - 1029) % 1024;

        // Otevření souboru ke čtení a zápisu
        FILE *indirect2_file = fopen(filename, "r+b");

        // Získání adresy ukazatele na datablok - úroven 1
        int32_t indirect2_level1_address = inode_ptr->indirect2 + (indirect2_level1_write_index * sizeof(int32_t));

        // Alokace paměti pro čtení
        int32_t *indirect2_level1_data = malloc(sizeof(int32_t));
        // Nulování paměti
        memset(indirect2_level1_data, 0, sizeof(int32_t));
        // Nastavení místa čtení v souboru
        fseek(indirect2_file, indirect2_level1_address, SEEK_SET);
        // Čtení dat
        fflush(indirect2_file);
        fread(indirect2_level1_data, sizeof(int32_t), 1, indirect2_file);

        // Alokace clusteru pokud je odkaz NULL
        if(*indirect2_level1_data == 0){
            int32_t indirect2_level1_allocation_index = bitmap_find_free_cluster_index(filename);

            if (indirect2_level1_allocation_index < 0) {
                log_debug("inode_add_data_address: Nelze alokovat 2. neprimou adresu úrovně 1, nedostatek volnych clusteru!\n");
                return -6;
            }

            int32_t indirect2_level1_allocation_address = bitmap_index_to_cluster_address(filename,
                                                                                          indirect2_level1_allocation_index);
            bitmap_set(filename, indirect2_level1_allocation_index, 1, TRUE);

            // Nulování datového bloku
            allocation_clear_cluster(filename, indirect2_level1_allocation_address);

            // Zápis do inode
            fseek(indirect2_file, indirect2_level1_address, SEEK_SET);
            fwrite(&indirect2_level1_allocation_address, sizeof(indirect2_level1_allocation_address), 1, indirect2_file);
            fflush(indirect2_file);

            // Zápis na VFS
            inode_write_to_index(filename, inode_ptr->id - 1, inode_ptr);
            log_trace("inode_add_data_address: Hodnota odkazu inode ID=%d indirect2[%d] nastavena na %d \n", inode_ptr->id, indirect2_level1_write_index,
                      indirect2_level1_allocation_address);
        }

        // Přečtení znovu jako pojistka
        // Nulování paměti
        memset(indirect2_level1_data, 0, sizeof(int32_t));
        // Nastavení místa čtení v souboru
        fseek(indirect2_file, indirect2_level1_address, SEEK_SET);
        // Čtení dat
        fread(indirect2_level1_data, sizeof(int32_t), 1, indirect2_file);

        // Povedlo se zapsat
        if(*indirect2_level1_data != 0){
            // Adresa kam zapsat
            int32_t indirect2_level2_address = *indirect2_level1_data + (indirect2_level2_write_index * sizeof(int32_t));

            // Zápis adresy na level 2
            fseek(indirect2_file, indirect2_level2_address, SEEK_SET);
            fwrite(&address, sizeof(int32_t), 1, indirect2_file);
            fflush(indirect2_file);

            // Logování
            log_trace("inode_add_data_address: Adresa databloku ulozena do indirect2[%d][%d] (addr: %d, value: %d, pointer_index: %d)\n", indirect2_level1_write_index, indirect2_level2_write_index, indirect2_level2_address, address, inode_ptr->allocated_clusters);
            address_writen = TRUE;

        }


        fclose(indirect2_file);
        free(indirect2_level1_data);
    }

    // Zabrání data bloku v bitmapě
    if(address_writen == TRUE){
        int32_t data_index_claimed = inode_data_index_from_address(filename, address);
        bitmap_set(filename, data_index_claimed, 1, TRUE);
        inode_ptr->allocated_clusters++;
        inode_write_to_index(filename, inode_ptr->id - 1, inode_ptr);
        free(superblock_ptr);
        return 0;
    }
    else{
        free(superblock_ptr);
        return -7;
    }

    /*
     * 0 - 4: directX+1
     * 5 - 1028: indirect1[X-5]
     * 1029 - END: indirect2[(X-1028)/1024][(X-1030)%1024]
     */
}