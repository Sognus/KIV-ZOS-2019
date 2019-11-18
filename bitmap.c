#include "bitmap.h"
#include <string.h>
#include <stdlib.h>
#include "parsing.h"
#include "superblock.h"
#include "bool.h"

/**
 * Vypíše řádkovou reprezentaci bitmapy
 *
 * @param filename soubor VFS
 */
int32_t bitmap_print(char *filename){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        log_debug("bitmap_print: Nelze pouzit prazdne jmeno souboru!\n");
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        log_debug("bitmap_print: Zadny soubor neexistuje!\n");
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        log_debug("bitmap_print: Nepodarilo se precist superblok!\n");
        return -3;
    }

    // Otevření souboru pro čtení
    FILE *file = fopen(filename, "r");

    // Ověření otevření souboru
    if(file == NULL) {
        log_debug("bitmap_print: Nepodarilo se otevrit soubor ke cteni!\n");
        return -4;
    }

    int32_t bitmap_address = superblock_ptr->bitmap_start_address;
    int32_t inode_address = superblock_ptr->inode_start_address;
    int32_t clusters = superblock_ptr->cluster_count;
    int32_t current_address = bitmap_address;
    bool *bitmap_data = malloc(sizeof(bool));

    printf("Bitmap: ");
    while(clusters > 0){
        fseek(file, current_address, SEEK_SET);
        fread(bitmap_data, sizeof(bool), 1, file);

        // Výpis
        printf("%d", *bitmap_data);

        clusters--;
        current_address = current_address + sizeof(bool);
    }
    printf("\n");

    // Uvolnění zdrojů
    fclose(file);
    free(bitmap_data);
    free(superblock_ptr);

    return TRUE;
}

/**
 * Nastaví souvislý blok hodnot v bitmapě na zvolenou hodnotu,
 * funkce neřeší kolizi již existujících dat.
 *
 * @param filename soubor vfs
 * @param index počáteční index zápisu
 * @param count počet členů v bloku
 * @param value hodnota
 * @return výsledek operace (return < 0 - chyby  | 0 - úspěch | return > 0 - kolik zápisů se nepodařilo)
 */
int32_t bitmap_set(char *filename, int32_t index, int32_t count, bool value){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        log_debug("bitmap_set: Nelze pouzit prazdne jmeno souboru!\n");
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        log_debug("bitmap_set: Zadany soubor neexistuje!\n");
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        log_debug("bitmap_set: Nepodarilo se precist superblok!\n");
        return -3;
    }

    // Otevření souboru pro zápis - speciálně r+b kvůli přepisování dat
    FILE *file = fopen(filename, "r+b");

    // Ověření otevření souboru
    if(file == NULL) {
        log_debug("bitmap_set: Nepodarilo se otevrit soubor ke cteni a zapisu!\n");
        return -4;
    }

    int32_t bitmap_address = superblock_ptr->bitmap_start_address;
    int32_t to_write = count;
    int32_t visit = 0;

    while(visit < superblock_ptr->cluster_count){
        if(visit == index){
            int32_t current_address = bitmap_address + sizeof(bool) * index;
            while(to_write && visit < superblock_ptr->cluster_count){
                fseek(file, current_address, SEEK_SET);
                fwrite(&value, sizeof(bool), 1, file);

                to_write--;
                visit++;
                current_address = current_address + sizeof(bool);
            }
        }


        visit++;
    }

    // Uvolnění zdrojů
    free(superblock_ptr);
    fclose(file);

    return to_write;
}

/**
 * Vrátí hodnotu bitmapy na určeném indexu
 *
 * @param filename soubor VFS
 * @param index index dat
 * @return hodnota dat na indexu (return < 0 - chyba)
 */
bool bitmap_get(char *filename, int32_t index) {
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        log_debug("bitmap_get: Nelze pouzit prazdne jmeno souboru!\n");
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        log_debug("bitmap_get: Zadany soubor neexistuje!\n");
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        log_debug("bitmap_get: Nepodarilo se precist superblok!\n");
        return -3;
    }

    // Ověření validity čtení
    if(index < 0 || index > superblock_ptr->cluster_count){
        log_debug("bitmap_get: Index je mimo povoleny rozsah!\n");
        return -4;
    }

    // Otevření souboru pro zápis - speciálně r+b kvůli přepisování dat
    FILE *file = fopen(filename, "r");

    // Ověření otevření souboru
    if(file == NULL) {
        log_debug("bitmap_get: Nepodarilo se otevrit soubor ke cteni!\n");
        return -5;
    }

    bool value = -6;
    int32_t read_address = superblock_ptr->bitmap_start_address + index * sizeof(bool);
    fseek(file, read_address, SEEK_SET);
    fread(&value, sizeof(bool), 1, file);

    // Uvolnění zdrojů
    free(superblock_ptr);
    fclose(file);

    return value;
}

/**
 * Na základě indexu vypočte počáteční adresu clusteru
 *
 * @param filename soubor VFS
 * @param index index clusteru
 * @return (return < 0 - chyba | return > 0 - adresa clusteru ve VFS)
 */
int32_t bitmap_index_to_cluster_address(char *filename, int32_t index){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        log_debug("bitmap_index_to_cluster_address: Nelze pouzit prazdne jmeno souboru!\n");
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        log_debug("bitmap_index_to_cluster_address: Zadany soubor neexistuje!\n");
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        log_debug("bitmap_index_to_cluster_address: Nepodarilo se precist superblok!\n");
        return -3;
    }

    // Ověření validity čtení
    if(index < 0 || index > superblock_ptr->cluster_count){
        log_debug("bitmap_index_to_cluster_address: Index je mimo povoleny rozsah!\n");
        return -4;
    }

    int32_t address = superblock_ptr->data_start_address + (index * superblock_ptr->cluster_size);

    free(superblock_ptr);
    return address;

}

/**
 * Vrátí první volný cluster v bitmapě
 *
 * @param filename soubor vfs
 * @return  index volného clusteru
 */
int32_t bitmap_find_free_cluster_index(char *filename){
    // Kontrola délky názvu souboru
    if(strlen(filename) < 1){
        log_debug("bitmap_find_free_cluster_index: Nelze pouzit prazdne jmeno souboru!\n");
        return -1;
    }

    // Ověření existence souboru
    if(file_exist(filename) != TRUE){
        log_debug("bitmap_find_free_cluster_index: Zadany soubor neexistuje!\n");
        return -2;
    }

    // Získání superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(filename);

    // Ověření ziskání superbloku
    if(superblock_ptr == NULL){
        log_debug("bitmap_find_free_cluster_index: Nepodarilo se precist superblok!\n");
        return -3;
    }

    int32_t current_index = 0;
    int32_t cluster_count = superblock_ptr->cluster_count;
    free(superblock_ptr);

    // Lineární prohledávání bitmapy
    while(current_index < cluster_count - 1){
        // Cluster je prázdný
        if(bitmap_get(filename, current_index) == FALSE){
            return current_index;
        }
        current_index++;
    }

    // Neexistuje volný cluster
    return -4;

}