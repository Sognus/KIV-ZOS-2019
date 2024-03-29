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

    // Vytvoření struktury INODE
    struct inode *inode_ptr = malloc(sizeof(struct inode));
    // Nulování struktury INODE
    memset(inode_ptr, 0, sizeof(struct inode));
    // Naplnění struktury daty - index je od 0 ale ID od 1 => id = index +1
    inode_ptr->id = inode_free_index + 1;
    // Nastavení typu INODE jako složka
    inode_ptr->type = VFS_DIRECTORY;
    // Zápis inode do VFS
    inode_write_to_index(vfs_filename, inode_free_index, inode_ptr);
    // Uvolnění paměti
    free(inode_ptr);


    // Speciální případ pro root složku
    if(strcmp(path, "/") == 0){
        // Ověření zda root složka již existuje
        if(inode_free_index != 0){
            log_debug("directory_create: Korenova slozka jiz existuje!");
            return -7;
        }

        parrent_id = 1;
        current_id = 1;
        strcpy(parrent_name, "..");
        strcpy(current_name, ".");
        vfs_file = vfs_open_inode(vfs_filename, 1);
    }
    else{
        // Zjištění prefix cesty
        char *path_prefix = get_prefix_string_until_last_character(path, "/");
        // Zjištění suffix cesty - název složky
        char *dir_name = get_suffix_string_after_last_character(path, "/");

        // Vytvoření záznamu
        struct directory_entry *parrent_entry = malloc(sizeof(struct directory_entry));
        memset(parrent_entry, 0, sizeof(struct directory_entry));
        parrent_entry->inode_id = inode_free_index + 1;
        strcpy(parrent_entry->name, dir_name);

        // TODO: změnit zápis záznamu do rodiče na funkci, projít data složky a hledat volné místo

        VFS_FILE *vfs_parrent = vfs_open_recursive(vfs_filename, path_prefix, 0);
        int32_t add_result = directory_add_entry(vfs_parrent, parrent_entry);
        log_debug("directory_create: Entry add result -> %d\n", add_result);

        if(vfs_parrent == NULL || add_result < 0) {
            free(parrent_entry);
            free(dir_name);
            free(path_prefix);
            free(parrent_name);
            free(current_name);
            return -8;
        }

        parrent_id = vfs_parrent->inode_ptr->id;
        current_id = inode_free_index + 1;
        strcpy(parrent_name, "..");
        strcpy(current_name, ".");

        // Uvolnění zdrojů
        vfs_close(vfs_parrent);
        free(parrent_entry);
        free(dir_name);
        free(path_prefix);

        vfs_file = vfs_open_inode(vfs_filename, current_id);
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
        vfs_file = vfs_open_recursive(vfs_filename, path, 0);
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


        if(strcmp(entry->name, "..") == 0 || strcmp(entry->name, ".") == 0){
            // Výpis
            printf("%s\t%d\n", entry->name, entry->inode_id);
        }
        else {
            // Char type
            VFS_FILE *entry_file = vfs_open_inode(vfs_filename, entry->inode_id);

            if(entry_file != NULL){
                char *type = filetype_to_short(entry_file->inode_ptr->type);

                // Výpis
                printf("%s%s\t%d\n", type, entry->name, entry->inode_id);


                // Uvolnění zdrojů
                vfs_close(entry_file);
                free(type);
            }

        }

        // Posun na další prvek - vlastně jen pro podmínku, SEEK se upravuje v VFS_READ
        current += sizeof(struct directory_entry);
    }

    // Uvolnění dat
    free(entry);
    vfs_close(vfs_file);

    // OK
    return 0;
}

/**
 * Při nalezení záznamu ve složce vrátí INODE ID záznamu, v opačném případě vrátí
 * hodnotu menší než 1
 *
 * @param vfs_filename cesta k VFS souboru
 * @param inode_id aktualně prohledávaná složka (její inode ID(
 * @param entry_name hledané jméno
 * @return výsledek operace (return < 0: chyba | return==0: Nenalezeno | return >0: Nalezeno)
 */
int32_t directory_has_entry(char *vfs_filename, int32_t inode_id ,char *entry_name){
    // Ověřování NULL
    if(vfs_filename == NULL){
        log_debug("directory_has_entry: Argument vfs_filename nemuze byt NULL!\n");
        return -1;
    }

    // Kontrola délky názvu souboru
    if(strlen(vfs_filename) < 1){
        log_debug("directory_has_entry: Argument vfs_filename nemuze byt prazdnym retezcem!\n");
        return -2;
    }

    if(inode_id < 1){
        log_debug("directory_has_entry: Soubor s INODE ID=%d nemuze existovat!\n", inode_id);
        return -3;
    }

    if(entry_name == NULL){
        log_debug("directory_has_entry: Argument entry_name nemuze byt NULL!\n");
        return -4;
    }

    if(strlen(entry_name) < 1) {
        log_debug("directory_has_entry: Argument entry_name nemuze byt prazdnym retezcem!\n");
        return -5;
    }

    // Ziskani INODE podle ID
    struct inode *inode_ptr = inode_read_by_index(vfs_filename, inode_id - 1);

    // Ověření ziskani ukazatele na inode
    if(inode_ptr == NULL){
        log_debug("directory_has_entry: Nelze ziskat ukazatel pro INODE ID=%d!\n", inode_id);
        return -6;
    }

    // Pokud jde o soubor nelze číst
    if(inode_ptr->type == VFS_FILE_TYPE){
        free(inode_ptr);
        log_debug("directory_has_entry: Ocekavana slozka, INODE ID=%d je soubor!\n", inode_id);
        return -7;
    }

    // Pokud máme symlink, potřebujeme dereferencovat a ověřit, zda ukazuje na složku
    // TODO: symlinkovaná složka

    // Můžeme číst složku - otevřeme
    VFS_FILE *vfs_file = vfs_open_inode(vfs_filename, inode_id);
    // Nastavíme offset na začátek
    vfs_seek(vfs_file, 0, SEEK_SET);

    struct directory_entry *entry = malloc(sizeof(struct directory_entry));
    int32_t curr = 0;
    while(curr + sizeof(struct directory_entry) <= vfs_file->inode_ptr->file_size){
        memset(entry, 0, sizeof(struct directory_entry));
        vfs_read(entry, sizeof(struct directory_entry), 1, vfs_file);

        // Nalezeni retezce
        if(strcmp(entry->name, entry_name) == 0) {
            int32_t entry_id = entry->inode_id;
            free(entry);
            free(inode_ptr);
            vfs_close(vfs_file);
            return entry_id;
        }

        curr += sizeof(struct directory_entry);
    }

    // Nepodařilo se nalézt entry s daným jménem;
    vfs_close(vfs_file);
    free(inode_ptr);
    free(entry);
    return 0;

}

/**
 * Přidá záznam directory_entry do souboru
 *
 * @param vfs_filename cesta k VFS souboru
 * @param path cesta uvnitř VFS
 * @param entry zapisovaný záznam
 * @return výsledek operace (return < 0: chyba | return >= 0: OK)
 */
int32_t directory_add_entry(VFS_FILE *vfs_parrent, struct directory_entry *entry){
    if(vfs_parrent == NULL){
        log_debug("directory_add_entry: parametr VFS soubor nemuze byt NULL!\n");
        return -1;
    }

    struct directory_entry *read_entry = malloc(sizeof(struct directory_entry));
    int32_t curr = 0;
    while(curr + sizeof(struct directory_entry) <= vfs_parrent->inode_ptr->file_size){
        memset(read_entry, 0, sizeof(struct directory_entry));
        vfs_read(read_entry, sizeof(struct directory_entry), 1, vfs_parrent);

        // Nalezeni retezce
        if(entry->inode_id == 0) {
            // Máme prázdný inode
            break;
        }

        curr += sizeof(struct directory_entry);
    }

    // Nastavení ukazatele
    vfs_seek(vfs_parrent, curr, SEEK_SET);
    // Zápis záznamu
    vfs_write(entry, sizeof(struct directory_entry), 1, vfs_parrent);

    // Uvolnění zdrojů
    free(read_entry);

    // OK
    return 0;
}

/**
 * Od aktuální inode provede přesun přes záznamy rodičů do root složky
 * a po cestě vytvoří cestu
 *
 * @param vfs_filename
 * @param inode_id
 * @param entry_name
 * @return
 */
char *directory_get_path(char *vfs_filename, int32_t inode_id) {
    if(vfs_filename == NULL){
        log_debug("directory_get_path: Parametr vfs_filename nemuze byt NULL!\n");
        return NULL;
    }

    if(strlen(vfs_filename) < 1){
        log_debug("directory_get_path: Parametr vfs_filename nemuze byt prazdny retezec!\n");
        return NULL;
    }

    struct inode *inode_ptr = inode_read_by_index(vfs_filename, inode_id - 1);

    if(inode_ptr == NULL) {
        log_debug("directory_get_path: Inode neexistuje!\n");
        return NULL;
    }

    char *path = malloc(sizeof(char) * 1 + 1);
    char *old_ptr = NULL;
    memset(path, 0, sizeof(char) * 1 + 1);
    strcpy(path, "/");
    int32_t curr_inode = inode_id;
    int32_t prev_inode = inode_id;

    while(1){
        VFS_FILE *vfs_file = vfs_open_inode(vfs_filename, curr_inode);

        if(vfs_file == NULL){
            free(inode_ptr);
            return NULL;
        }

        struct directory_entry *current = malloc(sizeof(struct directory_entry));
        struct directory_entry *parent = malloc(sizeof(struct directory_entry));
        memset(current, 0, sizeof(struct directory_entry));
        memset(parent, 0, sizeof(struct directory_entry));

        vfs_seek(vfs_file, 0, SEEK_SET);
        vfs_read(current, sizeof(struct directory_entry), 1, vfs_file);
        vfs_read(parent, sizeof(struct directory_entry), 1, vfs_file);

        // Otevření rodiče a zjištění jména
        VFS_FILE *vfs_parent = vfs_open_inode(vfs_filename, parent->inode_id);

        if(vfs_parent == NULL){
            free(current);
            free(parent);
            vfs_close(vfs_file);
            free(inode_ptr);
            return NULL;
        }

        // Jsme na root složce
        if(prev_inode == curr_inode && prev_inode == 1) {
            free(current);
            free(parent);
            vfs_close(vfs_file);
            vfs_close(vfs_parent);
            break;
        }

        struct directory_entry *entry = malloc(sizeof(struct directory_entry));
        int32_t curr = 0;
        while(curr + sizeof(struct directory_entry) <= vfs_parent->inode_ptr->file_size){
            memset(entry, 0, sizeof(struct directory_entry));
            vfs_read(entry, sizeof(struct directory_entry), 1, vfs_parent);


            // Nalezeni retezce
            if(entry->inode_id == curr_inode && curr_inode != 1) {
                old_ptr = path;
                path = str_prepend(entry->name, path);
                free(old_ptr);
                old_ptr = path;
                path = str_prepend("/", path);
                free(old_ptr);
            }

            // Jsme na root složce
            if(prev_inode == curr_inode && prev_inode == 1) {
                old_ptr = path;
                path = str_prepend("/", path);
                free(old_ptr);
                break;
            }

            curr += sizeof(struct directory_entry);
        }
        free(entry);

        // Posun dál
        prev_inode = curr_inode;
        curr_inode = parent->inode_id;


        free(current);
        free(parent);
        vfs_close(vfs_file);
        vfs_close(vfs_parent);


    }

    // Uvolnění zdrojů
    free(inode_ptr);

    return path;
}

/**
 * Zjistí inode rodičovské složky
 *
 * @param vfs_filename soubor VFS
 * @param inode_id aktuální složka k získání rodiče
 * @return (return < 1: ERR | return > 0: ID)
 */
int32_t directory_get_parent_id(char *vfs_filename, int32_t inode_id){
    if(vfs_filename == NULL){
        log_debug("directory_get_parent_id(: Parametr vfs_filename nemuze byt NULL!\n");
        return -1;
    }

    if(strlen(vfs_filename) < 1){
        log_debug("directory_get_parent_id(: Parametr vfs_filename nemuze byt prazdny retezec!\n");
        return -2;
    }

    // Pokus o otevření souboru
    VFS_FILE *vfs_file = vfs_open_inode(vfs_filename, inode_id);

    // Nepodařilo se otevřít INODE - pravděpodobně neexistuje
    if(vfs_file == NULL){
        return -3;
    }

    // TODO: symlink
    // Soubor není složka
    if(vfs_file->inode_ptr->type != VFS_DIRECTORY){
        vfs_close(vfs_file);
        return -4;
    }

    // Nastavení offsetu na 2. položku ve složce (1. je current ID)
    vfs_seek(vfs_file, 0 + sizeof(struct directory_entry), SEEK_SET);

    struct directory_entry *entry = malloc(sizeof(struct directory_entry));
    memset(entry, 0, sizeof(struct directory_entry));
    vfs_read(entry, sizeof(struct directory_entry), 1, vfs_file);

    int32_t entry_id = entry->inode_id;

    // Uvolnění zdrojů
    vfs_close(vfs_file);
    free(entry);

    // Návrat hodnoty
    return entry_id;
}

/**
 * Při nalezení záznamu ve složce vrátí záznam, v opačném případě vrátí NULL
 *
 * @param vfs_filename cesta k VFS souboru
 * @param inode_id aktualně prohledávaná složka (její inode ID(
 * @param entry_name hledané jméno
 * @return výsledek operace (struct directory_entry * | NULL)
 */
struct directory_entry *directory_get_entry(char *vfs_filename, int32_t inode_id ,char *entry_name){
    // Ověřování NULL
    if(vfs_filename == NULL){
        log_debug("directory_get_entry: Argument vfs_filename nemuze byt NULL!\n");
        return NULL;
    }

    // Kontrola délky názvu souboru
    if(strlen(vfs_filename) < 1){
        log_debug("directory_get_entry: Argument vfs_filename nemuze byt prazdnym retezcem!\n");
        return NULL;
    }

    if(inode_id < 1){
        log_debug("directory_get_entry: Soubor s INODE ID=%d nemuze existovat!\n", inode_id);
        return NULL;
    }

    if(entry_name == NULL){
        log_debug("directory_get_entry: Argument entry_name nemuze byt NULL!\n");
        return NULL;
    }

    if(strlen(entry_name) < 1) {
        log_debug("directory_get_entry: Argument entry_name nemuze byt prazdnym retezcem!\n");
        return NULL;
    }

    // Ziskani INODE podle ID
    struct inode *inode_ptr = inode_read_by_index(vfs_filename, inode_id - 1);

    // Ověření ziskani ukazatele na inode
    if(inode_ptr == NULL){
        log_debug("directory_get_entry: Nelze ziskat ukazatel pro INODE ID=%d!\n", inode_id);
        return NULL;
    }

    // Pokud jde o soubor nelze číst
    if(inode_ptr->type == VFS_FILE_TYPE){
        free(inode_ptr);
        log_debug("directory_get_entry: Ocekavana slozka, INODE ID=%d je soubor!\n", inode_id);
        return NULL;
    }

    // Pokud máme symlink, potřebujeme dereferencovat a ověřit, zda ukazuje na složku
    // TODO: symlinkovaná složka

    // Můžeme číst složku - otevřeme
    VFS_FILE *vfs_file = vfs_open_inode(vfs_filename, inode_id);
    // Nastavíme offset na začátek
    vfs_seek(vfs_file, 0, SEEK_SET);

    struct directory_entry *entry = malloc(sizeof(struct directory_entry));
    int32_t curr = 0;
    while(curr + sizeof(struct directory_entry) <= vfs_file->inode_ptr->file_size){
        memset(entry, 0, sizeof(struct directory_entry));
        vfs_read(entry, sizeof(struct directory_entry), 1, vfs_file);

        // Nalezeni retezce
        if(strcmp(entry->name, entry_name) == 0) {
            free(inode_ptr);
            vfs_close(vfs_file);
            return entry;
        }

        curr += sizeof(struct directory_entry);
    }

    // Nepodařilo se nalézt entry s daným jménem;
    vfs_close(vfs_file);
    free(inode_ptr);
    free(entry);
    return NULL;

}

/**
 * Ověří zda cesta ke složkce existuje, pokud existuje
 * Ověří se, zda složka je prázdná,
 * Pokud složka je prázdná, dealokují se databloky (včetně
 * uvolnění bitmapy a inode index se označí jako volný
 *
 * @param vfs_filename CESTA k VFS souboru
 * @param path cesta uvnitř VFS souboru
 * @return (return < 0: chyba | return = 0: OK | return >0: Message)
 */
int32_t directory_delete(char *vfs_filename, char *path){
    // Ověřování NULL
    if(vfs_filename == NULL){
        log_debug("directory_delete: Argument vfs_filename nemuze byt NULL!\n");
        return -1;
    }

    // Kontrola délky názvu souboru
    if(strlen(vfs_filename) < 1){
        log_debug("directory_delete: Argument vfs_filename nemuze byt prazdnym retezcem!\n");
        return -2;
    }

    // Ověření existence souboru
    if(file_exist(vfs_filename) != TRUE){
        log_debug("directory_delete: Cesta k souboru VFS neexistuje!\n");
        return -3;
    }

    // Ověřování NULL pro cestu
    if(path == NULL){
        log_debug("directory_delete: Parametr path nemuze byt NULL!\n");
        return -4;
    }

    // Overeni na delku cesty
    if(strlen(path) < 1) {
        log_debug("directory_delete: Cesta ke slozce uvnitr VFS nemuze byt prazdnym retezcem!\n");
        return -5;
    }

    // Pokus o otevření souboru
    VFS_FILE *vfs_file = vfs_open(vfs_filename, path);

    // Soubor se nepodařilo otevřít
    if(vfs_file == NULL){
        log_info("directory_delete: Soubor uvnitr VFS nebyl otevren - neexistuje!\n");
        return 1;
    }

    // Soubor není složka
    if(vfs_file->inode_ptr->type != VFS_DIRECTORY){
        vfs_close(vfs_file);
        log_info("directory_delete: Soubor uvnitr VFS neni slozka!\n");
        return 2;
    }


    // ID rodičovské složky
    int32_t parent_id = directory_get_parent_id(vfs_filename, vfs_file->inode_ptr->id);

    // Otevření rodičovské složky
    VFS_FILE *vfs_parent = vfs_open_inode(vfs_filename, parent_id);

    // Počet podsložek a souborů ve složce
    int32_t count = (vfs_file->inode_ptr->file_size / sizeof(struct directory_entry)) - 2;

    // Je více než
    if(count > 0){
        vfs_close(vfs_file);
        vfs_close(vfs_parent);
        log_info("directory_delete: Slozka neni prazdna!\n");
        return 3;
    }

    // Zjištění kolikátý entry je slozka v rodiči
    char *folder_name = get_suffix_string_after_last_character(path, "/");

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

    // Uvolnění zdrojů
    vfs_close(vfs_file);
    vfs_close(vfs_parent);
    free(empty_inode);
    free(folder_name);

    // ALL OK
    return 0;
}


