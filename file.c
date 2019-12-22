#include "file.h"
#include "vfs_io.h"
#include <stdlib.h>
#include "directory.h"
#include "structure.h"
#include "allocation.h"

/**
 * Vytvoří soubor ve VFS
 *
 * @param vfs_filename cesta k VFS
 * @param path cesta k souboru
 * @return
 */
int32_t file_create(char *vfs_filename, char *path){
    // Ověřování NULL
    if(vfs_filename == NULL){
        log_debug("file_create: Argument vfs_filename nemuze byt NULL!\n");
        return -1;
    }

    // Kontrola délky názvu souboru
    if(strlen(vfs_filename) < 1){
        log_debug("file_create: Argument vfs_filename nemuze byt prazdnym retezcem!\n");
        return -2;
    }

    // Ověření existence souboru
    if(file_exist(vfs_filename) != TRUE){
        log_debug("file_create: Cesta k souboru VFS neexistuje!\n");
        return -3;
    }

    // Ověřování NULL pro cestu
    if(path == NULL){
        log_debug("file_create: Parametr path nemuze byt NULL!\n");
        return -4;
    }

    // Overeni na delku cesty
    if(strlen(path) < 1){
        log_debug("file_create: Cesta ke slozce uvnitr VFS nemuze byt prazdnym retezcem!\n");
        return -5;
    }

    // Zjištění prefix cesty
    char *path_prefix = get_prefix_string_until_last_character(path, "/");
    // Zjištění suffix cesty - název složky
    char *file_name = get_suffix_string_after_last_character(path, "/");

    printf("path_prefix: %s\n", path_prefix);
    printf("file_name: %s\n", file_name);

    VFS_FILE *dir = vfs_open(vfs_filename, path_prefix);

    // Rodičovská složka neexistuje
    if(dir == NULL){
        log_debug("file_create: Nelze otevrit rodicovskou slozku!\n");

        // Uvolnění zdrojů
        free(path_prefix);
        free(file_name);
        return -6;
    }

    // Nelze vytvorit soubor - prilis dlouhe jmeno
    if(strlen(file_name) > 12){
        log_debug("file_create: Nelze vytvorit soubor, prilis dlouhe jmeno");
        free(path_prefix);
        free(file_name);
        return -7;
    }

    int32_t exist = directory_has_entry(vfs_filename, dir->inode_ptr->id, file_name);

    // Soubor ve slozce neexistuje, je treba vytvorit zaznam
    if(exist < 1){
        // Vytvoření inode
        // Zjištění volného indexu pro INODE -> index != 0 => máme již root
        int32_t inode_free_index = inode_find_free_index(vfs_filename);

        if(inode_free_index < 0){
            log_info("file_create: Nelze vytvorit soubor -> nedostatek volnych INODE!\n");

            // Uvolnění zdrojů
            vfs_close(dir);
            free(path_prefix);
            free(file_name);
            return -6;
        }

        // Vytvoření struktury INODE
        struct inode *inode_ptr = malloc(sizeof(struct inode));
        // Nulování struktury INODE
        memset(inode_ptr, 0, sizeof(struct inode));
        // Naplnění struktury daty - index je od 0 ale ID od 1 => id = index +1
        inode_ptr->id = inode_free_index + 1;
        // Nastavení typu INODE jako složka
        inode_ptr->type = VFS_FILE_TYPE;
        // Zápis inode do VFS
        inode_write_to_index(vfs_filename, inode_free_index, inode_ptr);

        // Zápis záznamu do rodič složky
        struct directory_entry *entry = malloc(sizeof(struct directory_entry));
        memset(entry, 0, sizeof(struct directory_entry));
        entry->inode_id = inode_ptr->id;
        strcpy(entry->name, file_name);

        directory_add_entry(dir, entry);

        int32_t rtn_id = inode_ptr->id;

        // Uvolnění paměti
        free(entry);
        free(inode_ptr);
        // Uvolnění zdrojů
        vfs_close(dir);
        free(path_prefix);
        free(file_name);

        return rtn_id;

    }
    else{
        log_info("file_create: Soubor jiz existuje!\n");

        // Uvolnění zdrojů
        vfs_close(dir);
        free(path_prefix);
        free(file_name);

        return exist;
    }

}

/**
 * Vymaže soubor z VFS
 *
 * @param vfs_filename cesta k VFS
 * @param path cesta uvnitř VFS
 * @return (return < 0: chyba | return == 0: OK)
 */
int32_t file_delete(char *vfs_filename, char *path){
    // Ověřování NULL
    if(vfs_filename == NULL){
        log_debug("file_delete: Argument vfs_filename nemuze byt NULL!\n");
        return -1;
    }

    // Kontrola délky názvu souboru
    if(strlen(vfs_filename) < 1){
        log_debug("file_delete: Argument vfs_filename nemuze byt prazdnym retezcem!\n");
        return -2;
    }

    // Ověření existence souboru
    if(file_exist(vfs_filename) != TRUE){
        log_debug("file_delete: Cesta k souboru VFS neexistuje!\n");
        return -3;
    }

    // Ověřování NULL pro cestu
    if(path == NULL){
        log_debug("file_delete: Parametr path nemuze byt NULL!\n");
        return -4;
    }

    // Overeni na delku cesty
    if(strlen(path) < 1){
        log_debug("file_delete: Cesta ke slozce uvnitr VFS nemuze byt prazdnym retezcem!\n");
        return -5;
    }

    // Zjištění prefix cesty
    char *path_prefix = get_prefix_string_until_last_character(path, "/");
    // Zjištění suffix cesty - název složky
    char *file_name = get_suffix_string_after_last_character(path, "/");

    // Otevření souboru
    VFS_FILE *vfs_file = vfs_open(vfs_filename, path);

    if(vfs_file == NULL){
        free(path_prefix);
        free(file_name);
        log_debug("file_delete: Nelze otevrit soubor ke smazani!");
        return -5;
    }


    if(vfs_file->inode_ptr->type == VFS_DIRECTORY){
        free(path_prefix);
        free(file_name);
        log_debug("file_delete: Cilovy soubor je slozka - nelze smazat pomoci rm!");
        vfs_close(vfs_file);
        return -10;
    }

    // Otevření rodiče
    VFS_FILE *vfs_parent = vfs_open(vfs_filename, path_prefix);

    if(vfs_parent == NULL){
        free(path_prefix);
        free(file_name);
        vfs_close(vfs_file);
        log_debug("file_delete: Nelze otevrit rodicovska slozka!");
        return -5;
    }

    // Vymazání záznamu v rodiči
    // Alokace dat pro entry
    struct directory_entry *entry = malloc(sizeof(struct directory_entry));

    // Čtení directory entry do velikosti souboru
    int32_t current_index = 0;
    int32_t current = 0;
    while(current + sizeof(struct directory_entry) <= vfs_parent->inode_ptr->file_size){
        memset(entry, 0, sizeof(struct directory_entry));
        vfs_read(entry, sizeof(struct directory_entry), 1, vfs_parent);

        if(entry->inode_id == vfs_file->inode_ptr->id){
            break;
        }

        // Posun na další prvek - vlastně jen pro podmínku, SEEK se upravuje v VFS_READ
        current += sizeof(struct directory_entry);
        current_index = current_index + 1;
    }
    free(entry);

    // Kolik má rodič záznamů
    int32_t parent_count = (vfs_parent->inode_ptr->file_size / sizeof(struct directory_entry));
    int32_t last_parent_entry_index = parent_count - 1;

    /*
     * Je potřeba posunout záznam
     * Posouváme poslední záznam na místo mazaného
     * Zmenšujeme velikost složky o 1 entry
     */
    if(last_parent_entry_index != current_index){
        int64_t seek = sizeof(struct directory_entry) * last_parent_entry_index;
        vfs_seek(vfs_parent, seek, SEEK_SET);

        // Přečtení poslední entry
        struct directory_entry *replace_entry = malloc(sizeof(struct directory_entry));
        memset(replace_entry, 0, sizeof(struct directory_entry));
        vfs_read(replace_entry, sizeof(struct directory_entry), 1, vfs_parent);

        // Seek na zápis
        int64_t seek_write = sizeof(struct directory_entry) * current_index;
        vfs_seek(vfs_parent, seek_write, SEEK_SET);
        vfs_write(replace_entry, sizeof(struct directory_entry), 1, vfs_parent);

        // Smazání staré entry
        vfs_seek(vfs_parent, seek, SEEK_SET);
        memset(replace_entry, 0, sizeof(struct directory_entry));
        vfs_write(replace_entry, sizeof(struct directory_entry), 1, vfs_parent);

        log_debug("directory_delete: Zaznam ve slozce %d presunut na %d\n", last_parent_entry_index, current_index);

        // Zmensen velikosti slozky o smazany zaznam
        vfs_parent->inode_ptr->file_size = vfs_parent->inode_ptr->file_size - sizeof(struct directory_entry);
        inode_write_to_index(vfs_filename, vfs_parent->inode_ptr->id - 1, vfs_parent->inode_ptr);

        free(replace_entry);
    }

    // Dealokování všech dat v INODE
    int32_t  dealloc_result = deallocate(vfs_filename, vfs_file->inode_ptr);

    // Smazat inode
    struct inode *empty_inode = malloc(sizeof(struct inode));
    memset(empty_inode, 0, sizeof(struct inode));
    inode_write_to_index(vfs_filename, vfs_file->inode_ptr->id - 1, empty_inode);

    free(path_prefix);
    free(file_name);
}

