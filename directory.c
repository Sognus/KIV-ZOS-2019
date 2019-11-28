#include "directory.h"
#include <string.h>
#include <stdlib.h>
#include "vfs_io.h"
#include "parsing.h"
#include "inode.h"
#include "structure.h"
#include "allocation.h"

/**
 * Vytvoří ve VFS novou složku
 *
 * @param vfs_filename cesta k VFS souboru
 * @param path cesta uvnitř VFS
 * @return výsledek operace (return < 0: chyba | return >=0: OK)
 */
int32_t directory_create(char *vfs_filename, char *path){
    // Ověřování NULL
    if(vfs_filename == NULL){
        log_debug("directory_create: Argument vfs_filename nemuze byt NULL!\n");
        return -1;
    }

    // Kontrola délky názvu souboru
    if(strlen(vfs_filename) < 1){
        log_debug("directory_create: Argument vfs_filename nemuze byt prazdnym retezcem!\n");
        return -2;
    }

    // Ověření existence souboru
    if(file_exist(vfs_filename) != TRUE){
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

    // Deklarace VFS_FILE
    VFS_FILE *vfs_file = NULL;

    // Deklarace základních dat nové složky
    int32_t parrent_id = -1;
    int32_t current_id = -1;

    // Alokace místa pro názvy složek
    char *parrent_name = malloc(sizeof(char) * 12);
    char *current_name = malloc(sizeof(char) * 12);
    memset(parrent_name, 0, sizeof(char) * 12);
    memset(current_name, 0, sizeof(char) * 12);

    // Vytvoření inode
    // Zjištění volného indexu pro INODE -> index != 0 => máme již root
    int32_t inode_free_index = inode_find_free_index(vfs_filename);
    // Ověření zda root složka již existuje
    if(inode_free_index != 0){
        log_debug("directory_create: Korenova slozka jiz existuje!");
        return -7;
    }

    // Vytvoření struktury INODE
    struct inode *inode_ptr = malloc(sizeof(struct inode));
    // Nulování struktury INODE
    memset(inode_ptr, 0, sizeof(struct inode));
    // Naplnění struktury daty - index je od 0 ale ID od 1 => id = index +1
    inode_ptr->id = inode_free_index + 1;
    // Nastavení typu INODE jako složka
    inode_ptr->type = VFS_DIRECTORY;
    // Zápis inode do VFS
    inode_write_to_index(vfs_filename, 0, inode_ptr);
    // Uvolnění paměti
    free(inode_ptr);


    // Speciální případ pro root složku
    if(strcmp(path, "/") == 0){
        parrent_id = 1;
        current_id = 1;
        strcpy(parrent_name, "..");
        strcpy(current_name, ".");
        vfs_file = vfs_open_inode(vfs_filename, 1);
    }
    else{
        // TODO: implement
    }

    if(vfs_file == NULL){
        log_debug("directory_create: Nelze otevrit VFS_FILE\n");
        return -6;
    }

    // Nastavení ukazatele
    vfs_seek(vfs_file, 0, SEEK_SET);

    // Vytvoření struktury
    struct directory_entry *entry = malloc(sizeof(struct directory_entry));
    memset(entry, 0, sizeof(struct directory_entry));
    entry->inode_id = current_id;
    strcpy(entry->name, current_name);

    // Zápis záznamu aktuální složky
    vfs_write(entry, sizeof(struct directory_entry), 1, vfs_file);

    memset(entry, 0, sizeof(struct directory_entry));
    entry->inode_id = parrent_id;
    strcpy(entry->name, parrent_name);

    // Zápis záznamu rodiče
    vfs_write(entry, sizeof(struct directory_entry), 1, vfs_file);

    // Uvolnění zdrojů
    vfs_close(vfs_file);
    free(parrent_name);
    free(current_name);
    free(entry);

    // OK
    return 0;
}

/**
 * Vypíše obsah složky ve VFS s danou cestou (příkaz LS)
 * @param vfs_filename cesta k VFS souboru
 * @param path cesta uvnitř VFS
 * @return výsledek operace (return < 0: chyba | return >=0: OK)
 */
int32_t directory_entries_print(char *vfs_filename, char *path){
    // Ověřování NULL
    if(vfs_filename == NULL){
        log_debug("directory_entries_print: Argument vfs_filename nemuze byt NULL!\n");
        return -1;
    }

    // Kontrola délky názvu souboru
    if(strlen(vfs_filename) < 1){
        log_debug("directory_entries_print: Argument vfs_filename nemuze byt prazdnym retezcem!\n");
        return -2;
    }

    // Ověření existence souboru
    if(file_exist(vfs_filename) != TRUE){
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

    VFS_FILE *vfs_file = NULL;

    // Speciální případ /
    if(strcmp(path, "/") == 0){
        vfs_file = vfs_open_inode(vfs_filename, 1);
    } else {
        // TODO: implement
    }

    if(vfs_file == NULL){
        log_debug("directory_create: Nelze otevrit VFS_FILE\n");
        return -6;
    }

    // Nastavení ukazatele
    vfs_seek(vfs_file, 0, SEEK_SET);

    // Alokace dat pro entry
    struct directory_entry *entry = malloc(sizeof(struct directory_entry));

    printf("+DIRECTORY\n");

    // Čtení directory entry do velikosti souboru
    int32_t current = 0;
    while(current + sizeof(struct directory_entry) <= vfs_file->inode_ptr->file_size){
        memset(entry, 0, sizeof(struct directory_entry));
        vfs_read(entry, sizeof(struct directory_entry), 1, vfs_file);

        // Výpis
        printf("%s\t%d\n", entry->name, entry->inode_id);

        // Posun na další prvek - vlastně jen pro podmínku, SEEK se upravuje v VFS_READ
        current += sizeof(struct directory_entry);

    }

    // Uvolnění dat
    free(entry);
    vfs_close(vfs_file);

    // OK
    return 0;
}


