#include "inode.h"
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "superblock.h"
#include "parsing.h"
#include "bitmap.h"
#include "allocation.h"

/**
 * Vypíše obsah struktury inode
 *
 * @param ptr ukazatel na strukturu inode
 */
void inode_print(struct inode *ptr){
    if (ptr == NULL) {
        log_trace("Ukazatel na strukturu superblock je NULL!\n");
        return;
    }

    log_info("*** INODE\n");
    log_info("ID: %d\n", ptr->id);
    log_info("Type: %d\n", ptr->type);
    log_info("References: %d\n", ptr->references);
    log_info("File size: %d\n", ptr->file_size);
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
        return -4;
    }

    // Otevření souboru pro čtení
    FILE *file = fopen(filename, "r+b");

    // Ověření otevření souboru
    if(file == NULL){
        return -5;
    }


    // Nastavení ukazatele zápisu
    fseek(file, inode_address, SEEK_SET);
    fflush(file);
    fwrite(inode_ptr, sizeof(struct inode), 1, file);
    fclose(file);

    // Akce se podařila
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
        return NULL;
    }

    // Otevření souboru pro čtení
    FILE *file = fopen(filename, "rb");

    // Ověření otevření souboru
    if(file == NULL){
        return NULL;
    }

    struct inode *inode_ptr = malloc(sizeof(struct inode));
    fseek(file, inode_address, SEEK_SET);
    fread(inode_ptr, sizeof(struct inode), 1, file);
    fclose(file);

    //Inode je prázdná
    if(inode_ptr->id == 0){
        return NULL;
    }

    return inode_ptr;
}

/**
 * TODO: Přesun do allocation
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
        return -4;
    }

    return index;
}

/**
 * Přidá nový ukazatel na datový blok pro strukturu
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
        return -4;
    }

    // TODO: Kontrola jestli už je adresa/index použitá (možná)

    bool address_writen = FALSE;
    int32_t index_written = -50;

    /*
     * Kontrola možnosti zapsání přímých adres
     */

    if(inode_ptr->direct1 == 0 && address_writen == FALSE){
        inode_ptr->direct1 = address;
        index_written = 1;
        address_writen = TRUE;
        log_trace("inode_add_data_address: Adresa databloku ulozena do direct1 (index 1)\n");
    }

    if(inode_ptr->direct2 == 0 && address_writen == FALSE){
        inode_ptr->direct2 = address;
        index_written = 2;
        address_writen = TRUE;
        log_trace("inode_add_data_address: Adresa databloku ulozena do direct2 (index 2)\n");
    }

    if(inode_ptr->direct3 == 0 && address_writen == FALSE){
        inode_ptr->direct3 = address;
        index_written = 3;
        address_writen = TRUE;
        log_trace("inode_add_data_address: Adresa databloku ulozena do direct3 (index 3)\n");
    }

    if(inode_ptr->direct4 == 0 && address_writen == FALSE){
        inode_ptr->direct4 = address;
        index_written = 4;
        address_writen = TRUE;
        log_trace("inode_add_data_address: Adresa databloku ulozena do direct4 (index 4)\n");
    }

    if(inode_ptr->direct5 == 0 && address_writen == FALSE){
        inode_ptr->direct5 = address;
        index_written = 5;
        address_writen = TRUE;
        log_trace("inode_add_data_address: Adresa databloku ulozena do direct5 (index 5)\n");
    }

    /*
     * 1. nepřímý odkaz na clustery
     */

    // Alokace pro nepřímý odkaz v případě, že je potřeba
    if(inode_ptr->indirect1 == 0 && address_writen == FALSE){
        int32_t indirect1_allocation_index = bitmap_find_free_cluster_index(filename);

        if(indirect1_allocation_index < 0){
            log_debug("inode_add_data_address: Nelze alokovat 1. neprimou adresu, nedostatek volnych clusteru!\n");
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
                log_trace("inode_add_data_address: Adresa databloku ulozena do indirect1 (index %d)\n", index_written);
                break;
            }

            // Posun na další adresu
            indirect1_iter_current += sizeof(int32_t);
            index_written++;
        }

        fclose(file_read);
        free(indirect1_iter_data);
    }

    // TODO: nepřímý odkaz 2
    if(address_writen == FALSE) {
        int32_t *indirect2_iter_data = malloc(sizeof(int32_t));

        // Alokace pro inode->indirect2 pokud je ukazatel NULL
        if (inode_ptr->indirect2 == 0) {
            int32_t indirect2_allocation_index = bitmap_find_free_cluster_index(filename);

            if (indirect2_allocation_index < 0) {
                log_debug("inode_add_data_address: Nelze alokovat 2. neprimou adresu, nedostatek volnych clusteru!\n");
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
            log_trace("inode_add_data_address: Hodnota 2. nepřímého odkazu pro ID=%d nastavena na %d\n", inode_ptr->id,
                      inode_ptr->indirect2);
        }
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
    // zbytek = indirect2
    return address_writen;
}
