#include "directory.h"
#include <string.h>
#include <stdlib.h>
#include "parsing.h"
#include "inode.h"
#include "structure.h"
#include "allocation.h"

/**
 * Vytvoří ve VFS novou složku
 *
 * @param vfs_file cesta k VFS souboru
 * @param path cesta uvnitř VFS
 * @return výsledek operace (return < 0: chyba | return >=0: OK)
 */
int32_t directory_create(char *vfs_file, char *path){
    // Ověřování NULL
    if(vfs_file == NULL){
        log_debug("directory_create: Argument vfs_filename nemuze byt NULL!\n");
        return -1;
    }

    // Kontrola délky názvu souboru
    if(strlen(vfs_file) < 1){
        log_debug("directory_create: Argument vfs_filename nemuze byt prazdnym retezcem!\n");
        return -2;
    }

    // Ověření existence souboru
    if(file_exist(vfs_file) != TRUE){
        log_debug("directory_create: Cesta k souboru VFS neexistuje!\n");
        return -3;
    }

    // Ověřování NULL pro cestu
    if(path == NULL){
        log_debug("directory_create: Parametr path nemuze byt NULL!\n");
        return -4;
    }

    // Overeni na delku cesty
    if(strlen(path) < 1){
        log_debug("directory_create: Cesta ke slozce uvnitr VFS nemuze byt prazdnym retezcem!\n");
        return -5;
    }

    // Speciální případ pro root složku
    if(strcmp(path, "/") == 0){

        // Zjištění volného indexu pro INODE -> index != 0 => máme již root
        int32_t inode_free_index = inode_find_free_index(vfs_file);
        // Ověření zda root složka již existuje
        if(inode_free_index != 0){
            log_debug("directory_create: Korenova slozka jiz existuje!");
            return -7;
        }

        // Otevreni souboru pro cteni
        FILE *file = fopen(vfs_file, "r+b");

        if(file == NULL) {
            log_error("directory_create: Nepodarilo se otevrit VFS\n!");
            return -6;
        }

        // Vytvoření struktury INODE
        struct inode *inode_ptr = malloc(sizeof(struct inode));
        // Nulování struktury INODE
        memset(inode_ptr, 0, sizeof(struct inode));
        // Naplnění struktury daty - index je od 0 ale ID od 1 => id = index +1
        inode_ptr->id = inode_free_index + 1;
        // Nastavení typu INODE jako složka
        inode_ptr->type = VFS_DIRECTORY;
        // Alokace 1 data bloku pro složku
        int32_t allocate_result = allocate_data_blocks(vfs_file, 1, inode_ptr);
        // Test alokace
        if(allocate_result != 0){
            free(inode_ptr);
            fclose(file);
            log_error("directory_create: Nepodařilo se alokovat datablok pro složku - vysledek operace %d\n", allocate_result);
            return -8;
        }


        // Zápis záznamu kořenové složky do databloku
        struct directory_entry *entry = malloc(sizeof(struct directory_entry));
        memset(entry, 0, sizeof(struct directory_entry));
        entry->inode_id = 1;
        strcpy(entry->name, ".\0");

        fseek(file, inode_ptr->direct1, SEEK_SET);
        fwrite(entry, sizeof(struct directory_entry), 1, file);

        // Uprava pro ..
        strcpy(entry->name, "..\0");
        fwrite(entry, sizeof(struct directory_entry), 1, file);

        // Zapis velikosti
        inode_ptr->file_size = 2 * sizeof(struct directory_entry);
        inode_write_to_index(vfs_file, 0, inode_ptr);

        // Uvolnění zdrojůf
        free(entry);
        fclose(file);
        free(inode_ptr);

    }
    else{
        // TODO: implement
    }


    return 0;
}

/**
 * Vypíše obsah složky ve VFS s danou cestou (příkaz LS)
 * @param vfs_file cesta k VFS souboru
 * @param path cesta uvnitř VFS
 * @return výsledek operace (return < 0: chyba | return >=0: OK)
 */
int32_t directory_entries_print(char *vfs_file, char *path){
    // Ověřování NULL
    if(vfs_file == NULL){
        log_debug("directory_entries_print: Argument vfs_filename nemuze byt NULL!\n");
        return -1;
    }

    // Kontrola délky názvu souboru
    if(strlen(vfs_file) < 1){
        log_debug("directory_entries_print: Argument vfs_filename nemuze byt prazdnym retezcem!\n");
        return -2;
    }

    // Ověření existence souboru
    if(file_exist(vfs_file) != TRUE){
        log_debug("directory_entries_print: Cesta k souboru VFS neexistuje!\n");
        return -3;
    }

    // Ověřování NULL pro cestu
    if(path == NULL){
        log_debug("directory_entries_print Parametr path nemuze byt NULL!\n");
        return -4;
    }

    // Overeni na delku cesty
    if(strlen(path) < 1){
        log_debug("directory_entries_print: Cesta ke slozce uvnitr VFS nemuze byt prazdnym retezcem!\n");
        return -5;
    }

    // Speciální případ /
    if(strcmp(path, "/") == 0){
        struct superblock *superblock_ptr = superblock_from_file(vfs_file);

        if(superblock_ptr == NULL){
            log_debug("directory_entries_print: Nelze ziskat superblock z VFS!\n");
            return -6;
        }

        int32_t inode_id = 1;
        struct inode *inode_ptr = inode_read_by_index(vfs_file, inode_id -  1);

        // Overeni existence
        if(inode_ptr == NULL){
            free(superblock_ptr);
            log_debug("directory_entries_print: Nelze precist inode pro korenovou slozku!\n");
            return -7;
        }

        // Otevření souboru VFS
        FILE *file = fopen(vfs_file, "r+b");

        // Otevření otevření souboru
        if(file == NULL){
            log_debug("directory_entries_print: Nelze cist soubor VFS!\n");
            return -7;
        }

        // Zpracování direct databloků
        int32_t directs[5] = {inode_ptr->direct1, inode_ptr->direct2, inode_ptr->direct3, inode_ptr->direct4, inode_ptr->direct5 };
        int32_t direct_index = 0;

        for(int i = 0; i < 5; i++){

            if (directs[i] != 0) {
                int32_t curr_addr = directs[i];
                int32_t end_addr = curr_addr + superblock_ptr->cluster_size;

                struct directory_entry *entry = malloc(sizeof(struct directory_entry));

                while (curr_addr < end_addr) {
                    // Nulování struktury
                    memset(entry, 0, sizeof(struct directory_entry));
                    // Nastavení adresy čtení
                    fseek(file, curr_addr, SEEK_SET);
                    // Čtení z VFS
                    fread(entry, sizeof(struct directory_entry), 1, file);
                    // Ukončení čtení v případě konce záznamů
                    if (entry->inode_id == 0) {
                        break;
                    }
                    // Ziskani inode
                    struct inode *read_inode = inode_read_by_index(vfs_file, entry->inode_id - 1);
                    // Vypis inode
                    char *filetype = filetype_to_name(read_inode->type);
                    printf("%s\t%s\t%d\n", filetype, entry->name, inode_id);


                    free(read_inode);
                    free(filetype);
                    curr_addr = curr_addr + sizeof(struct directory_entry);
                }

                free(entry);

            }
        }

        // Zpracování indirect1 databloků:
        if(inode_ptr->indirect1 != 0) {
            // TODO: implement
            log_error("directory_entries_print: Hodnota indirect1 neni NULL, ale zpracovani neni implementovano\n");
        }

        if(inode_ptr->indirect2 != 0) {
            // TODO: implement
            log_error("directory_entries_print: Hodnota indirect2 neni NULL, ale zpracovani neni implementovano\n");
        }


        free(inode_ptr);
        free(superblock_ptr);
        fclose(file);
    } else {
        // TODO: implement
    }
}


