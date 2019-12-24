#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "shell.h"
#include "debug.h"
#include "parsing.h"
#include "commands.h"
#include "structure.h"
#include "directory.h"
#include "file.h"

/**
 * Příkaz PWD
 * @param sh kontext virtuálního terminálu
 */
void cmd_pwd(struct shell *sh){
    if(sh == NULL){
        log_debug("cmd_pwd: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    char *path = directory_get_path(sh->vfs_filename, sh->cwd);
    printf("%s\n", path);
    free(path);
}

/**
 * Příkaz FORMAT
 *
 * @param sh kontext virtuálního terminálu
 * @param command příkaz
 */
void cmd_format(struct shell *sh, char *command) {
    if(sh == NULL){
        log_debug("cmd_format: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        printf("CANNOT CREATE FILE\n");
        return;
    }

    if(command == NULL){
        log_debug("cmd_format: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        printf("CANNOT CREATE FILE\n");
        return;
    }

    if(strlen(command) < 1){
        log_debug( "cmd_format: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        printf("CANNOT CREATE FILE\n");
        return;
    }

    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");
    // První parametr příkazu
    token = strtok(NULL, " ");

    int64_t size = parse_filesize(token);

    if(size != 0){
        // Vytvoření implicitního superbloku
        struct superblock *ptr = superblock_impl_alloc(size);
        // Výpočet hodnot superbloku dle velikosti disku
        structure_calculate(ptr);
        // Vytvoření virtuálního FILESYSTEMU
        vfs_create(sh->vfs_filename, ptr);
        // Vytvoření kořenové složky
        directory_create(sh->vfs_filename, "/");
        // Nastavení kontextu terminálu na root
        sh->cwd = 1;
        // Uvolnění zdrojů
        free(ptr);
        // Povinný výpis
        printf("OK\n");
    }else {
        printf("CANNOT CREATE FILE\n");
    }


}

/**
 * Příkaz: Změna složky
 *
 * @param sh kontext virtuálního terminálu
 * @param command příkaz
 */
void cmd_cd(struct shell *sh, char *command) {
    if (sh == NULL) {
        log_debug("cmd_cd: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    if (command == NULL) {
        log_debug("cmd_cd: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        return;
    }

    if (strlen(command) < 1) {
        log_debug("cmd_format: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        return;
    }

    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");
    // První parametr příkazu
    token = strtok(NULL, " ");

    // Odstranit \n
    if(token[strlen(token)-1] == '\n'){
        token[strlen(token)-1] = '\0';
    }

    if(strcmp(token, "/") == 0){
        sh->cwd = 1;
        return;
    }


    char *path_absolute = NULL;

    // Převod na absolutní cestu
    if(starts_with("/", token)){
        path_absolute = path_parse_absolute(sh, token);
    }else {
        char *cwd = directory_get_path(sh->vfs_filename, sh->cwd);
        char *mashed = str_prepend(cwd, token);
        path_absolute = path_parse_absolute(sh, mashed);
        free(mashed);
        free(cwd);
    }

    // Pokud se nám nepodaří zpracovat retezec, tak ukoncime zpracovani
    if(path_absolute == NULL){
        printf("PATH NOT FOUND (neexistujici cesta)\n");
        return;
    }

    // TODO: prelozeni symlinku

    // Pokus o otevření souboru
    VFS_FILE *vfs_file = vfs_open(sh->vfs_filename, path_absolute);

    // Pokud se podařilo otevřít soubor a soubor je složka, změníme CWD na cíl. složku
    if(vfs_file != NULL && vfs_file->inode_ptr->type == VFS_DIRECTORY){
        // Změna aktuální CWD
        sh->cwd = vfs_file->inode_ptr->id;
        printf("OK\n");

    }else {
        // Cesta neexistuje
        printf("PATH NOT FOUND (neexistujici cesta)\n");
    }

    // Podmíněné uvolnění zdrojů
    if(vfs_file != NULL) {
        vfs_close(vfs_file);
    }

    // Uvolneni zdrojů
    free(path_absolute);


}

/**
 * Příkaz: Vytvoření adresáře
 *
 * @param sh
 * @param command
 */
void cmd_mkdir(struct shell *sh, char *command) {
    if (sh == NULL) {
        log_debug("cmd_format: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    if (command == NULL) {
        log_debug("cmd_format: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        return;
    }

    if (strlen(command) < 1) {
        log_debug("cmd_format: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        return;
    }

    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");
    // První parametr příkazu
    token = strtok(NULL, " ");

    // Odstranit \n
    if(token[strlen(token)-1] == '\n'){
        token[strlen(token)-1] = '\0';
    }

    // TODO: Ověřit existenci souboru

    char *path_absolute = NULL;

    // Převod na absolutní cestu
    if(starts_with("/", token)){
        path_absolute = path_parse_absolute(sh, token);
    }else {
        char *cwd = directory_get_path(sh->vfs_filename, sh->cwd);
        char *mashed = str_prepend(cwd, token);
        path_absolute = path_parse_absolute(sh, mashed);
        free(mashed);
        free(cwd);
    }

    if(path_absolute == NULL){
        printf("PATH NOT FOUND (neexistuje zadaná cesta)\n");
        log_debug("cmd_mkdir: Nepodarilo se prevest cestu na absolutni");
        return;
    }

    // Existuje soubor
    VFS_FILE *vfs_exist = vfs_open(sh->vfs_filename, path_absolute);

    if(vfs_exist != NULL){
        vfs_close(vfs_exist);
        printf("EXIST\n");
        return;
    }

    int create_result = directory_create(sh->vfs_filename, path_absolute);

    if(create_result != 0){
        printf("mkdir error: code %d\n", create_result);
    }else{
        printf("OK\n");
    }

    free(path_absolute);
}

/**
 * Příkaz: Výpis složky
 *
 * Pokud command == null -> výpis aktuální složky
 *
 * @param sh
 * @param command
 */
void cmd_ls(struct shell *sh, char *command){
    if (sh == NULL) {
        log_debug("cmd_ls: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    if(command == NULL){
        char *absolute_path = directory_get_path(sh->vfs_filename, sh->cwd);
        directory_entries_print(sh->vfs_filename, absolute_path);
        free(absolute_path);
    }
    else{
        char *token = NULL;
        // Jméno příkazu
        token = strtok(command, " ");
        // První parametr příkazu
        token = strtok(NULL, " ");

        if(token == NULL){
            printf("PATH NOT FOUND (neexistujici adresar)\n");
            return;
        }

        // Odstranit \n
        if(token[strlen(token)-1] == '\n'){
            token[strlen(token)-1] = '\0';
        }

        char *path_absolute = NULL;

        // Převod na absolutní cestu
        if(starts_with("/", token)){
            path_absolute = path_parse_absolute(sh, token);
        }else {
            char *cwd = directory_get_path(sh->vfs_filename, sh->cwd);
            char *mashed = str_prepend(cwd, token);
            path_absolute = path_parse_absolute(sh, mashed);
            free(mashed);
            free(cwd);
        }

        printf("path_absolute: %s\n", path_absolute);

        // Výpis obsahu složky
        int print_result = directory_entries_print(sh->vfs_filename, path_absolute);

        if(print_result < 0){
            printf("PATH NOT FOUND (neexistujici adresar)\n");
        }

        free(path_absolute);
    }


}

/**
 * Příkaz: Nahrání souboru do VFS
 *
 *
 * @param sh
 * @param command
 */
void cmd_incp(struct shell *sh, char *command){
    if (sh == NULL) {
        log_debug("cmd_incp: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    if (command == NULL) {
        log_debug("cmd_incp: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        return;
    }

    if (strlen(command) < 1) {
        log_debug("cmd_incp: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        return;
    }


    char *first = NULL;
    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");

    // První parametr příkazu
    token = strtok(NULL, " ");
    if(token == NULL){
        printf("incp: First parameter is missing!\n");
        return;
    }
    first = malloc(sizeof(char) * strlen(token) + 1);
    strcpy(first, token);

    if(file_exist(first) == FALSE){
        free(first);
        printf("FILE NOT FOUND (neni zdroj)\n");
        return;
    }

    // Druhý parametr příkazu
    token = strtok(NULL, " ");
    if(token == NULL){
        free(first);
        printf("incp: Second parameter is missing!\n");
        return;
    }

    // Uprava posledniho parametru - odstraneni \n
    if(token[strlen(token)-1] == '\n'){
        token[strlen(token)-1] = '\0';
    }

    char *path_absolute = NULL;

    // Převod na absolutní cestu
    if(starts_with("/", token)){
        path_absolute = path_parse_absolute(sh, token);
    }else {
        char *cwd = directory_get_path(sh->vfs_filename, sh->cwd);
        char *mashed = str_prepend(cwd, token);
        path_absolute = path_parse_absolute(sh, mashed);
        free(mashed);
        free(cwd);
    }

    if(path_absolute == NULL){
        free(first);
        printf("PATH NOT FOUND (neexistuje cilova cesta) \n");
        log_debug("cmd_incp: Nepodarilo se prevest cestu na absolutni");
        return;
    }


    // Vytvoření souboru pokud je potřeba
    int32_t id = file_create(sh->vfs_filename, path_absolute);

    // Otevřeni ciloveho souboru
    VFS_FILE *target = vfs_open(sh->vfs_filename, path_absolute);

    if(target == NULL){
        printf("PATH NOT FOUND (neexistuje cilova cesta) \n");
        log_debug("cmd_incp: Cilove umisteni neexistuje!\n");
        free(first);
        return;
    }

    FILE *external = fopen(first, "r+b");

    if(external == NULL){
        free(first);
        printf("FILE NOT FOUND (neni zdroj)\n");
        NULL;
    }

    vfs_seek(target, 0, SEEK_SET);
    fseek(external, 0, SEEK_SET);

    int c = 0;
    while ((c = getc(external)) != EOF){
        int result = vfs_write(&c, 1, 1, target);
        if(result != 0){
            vfs_close(target);
            fclose(external);
            printf("PARTIAL WRITE\n");
            return;
        }
    }

    printf("OK\n");

    // Uvolnění zdrojů
    free(path_absolute);
    fclose(external);
    vfs_close(target);
    free(first);
}

/**
 * Příkaz: Vypsání obsahu souboru (ne složky)
 *
 * @param sh
 * @param command
 */
void cmd_cat(struct shell *sh, char *command){
    if (sh == NULL) {
        log_debug("cmd_cat: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    if (command == NULL) {
        log_debug("cmd_cat: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        return;
    }

    if (strlen(command) < 1) {
        log_debug("cmd_cat: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        return;
    }

    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");
    // První parametr příkazu
    token = strtok(NULL, " ");

    if(token == NULL){
        printf("FILE NOT FOUND (není zdroj)\n");
        log_debug("cmd_cat: Chybejici parametr prikazu!\n");
        return;
    }

    // Odstranit \n
    if(token[strlen(token)-1] == '\n'){
        token[strlen(token)-1] = '\0';
    }

    char *path_absolute = NULL;

    // Převod na absolutní cestu
    if(starts_with("/", token)){
        path_absolute = path_parse_absolute(sh, token);
    }else {
        char *cwd = directory_get_path(sh->vfs_filename, sh->cwd);
        char *mashed = str_prepend(cwd, token);
        path_absolute = path_parse_absolute(sh, mashed);
        free(mashed);
        free(cwd);
    }

    if(path_absolute == NULL){
        printf("FILE NOT FOUND (není zdroj)\n");
        log_debug("cmd_cat: Nelze prelozit cestu na absolutni!\n");
        return;
    }

    VFS_FILE *source = vfs_open(sh->vfs_filename, path_absolute);

    if(source == NULL){
        free(path_absolute);
        printf("FILE NOT FOUND (není zdroj)\n");
        log_debug("cmd_cat: Soubor neexistuje!\n");
        return;
    }

    // TODO: překlad symlinku

    if(source->inode_ptr->type != VFS_FILE_TYPE){
        free(path_absolute);
        printf("FILE NOT FOUND (není zdroj)\n");
        log_debug("cmd_cat: Soubor neexistuje!\n");

        vfs_close(source);

        return;
    }

    int32_t read_size = sizeof(char) * 256;
    int32_t read_done = 0;
    vfs_seek(source, 0, SEEK_SET);

    char *buffer = malloc(sizeof(char) * 256);

    while(read_done < source->inode_ptr->file_size){
        memset(buffer, 0, sizeof(char) * 256);
        vfs_read(buffer, sizeof(char), read_size, source);

        // Výpis obsahu
        printf("%s", buffer);

        read_done = read_done + read_size;
    }

    // Zarovnání pro terminál
    printf("\n");

    // Uvolnění zdrojů
    free(path_absolute);
    free(buffer);
    vfs_close(source);
}

/**
 * Příkaz: Smazání složky pokud je prázdná
 *
 * @param sh
 * @param command
 */
void cmd_rmdir(struct shell *sh, char *command){
    if(sh == NULL){
        log_debug("cmd_rmdir: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        printf("FILE NOT FOUND (neexistujici adresar)\n");
        return;
    }

    if(command == NULL){
        log_debug("cmd_rmdir: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        printf("FILE NOT FOUND (neexistujici adresar)\n");
        return;
    }

    if(strlen(command) < 1){
        log_debug( "cmd_rmdir: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        printf("FILE NOT FOUND (neexistujici adresar)\n");
        return;
    }

    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");
    // První parametr příkazu
    token = strtok(NULL, " ");

    // Odstranit \n
    if(token[strlen(token)-1] == '\n'){
        token[strlen(token)-1] = '\0';
    }

    char *path_absolute = NULL;

    // Převod na absolutní cestu
    if(starts_with("/", token)){
        path_absolute = path_parse_absolute(sh, token);
    }else {
        char *cwd = directory_get_path(sh->vfs_filename, sh->cwd);
        char *mashed = str_prepend(cwd, token);
        path_absolute = path_parse_absolute(sh, mashed);
        free(mashed);
        free(cwd);
    }

    // Pokud se nám nepodaří zpracovat retezec, tak ukoncime zpracovani
    if(path_absolute == NULL){
        printf("PATH NOT FOUND (neexistujici cesta)\n");
        return;
    }

    // Smazat složku
    int32_t delete_result = directory_delete(sh->vfs_filename, path_absolute);

    if(delete_result < 0){
        printf("FILE NOT FOUND (neexistujici adresar)\n");
    }

    if(delete_result == 3){
        printf("NOT EMPTY (adresar obsahuje podadresare, nebo soubory)\n");
    }

    if(delete_result == 0){
        printf("OK\n");
    }

    // Uvolnění zdrojů
    free(path_absolute);
}

/**
 *
 * Příkaz: smazání souboru
 *
 * @param sh
 * @param command
 */
void cmd_rm(struct shell *sh, char *command){
    if(sh == NULL){
        log_debug("cmd_rm: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        printf("FILE NOT FOUND\n");
        return;
    }

    if(command == NULL){
        log_debug("cmd_rm: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        printf("FILE NOT FOUND\n");
        return;
    }

    if(strlen(command) < 1){
        log_debug( "cmd_rm: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        printf("FILE NOT FOUND\n");
        return;
    }

    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");
    // První parametr příkazu
    token = strtok(NULL, " ");

    // Odstranit \n
    if(token[strlen(token)-1] == '\n'){
        token[strlen(token)-1] = '\0';
    }

    char *path_absolute = NULL;

    // Převod na absolutní cestu
    if(starts_with("/", token)){
        path_absolute = path_parse_absolute(sh, token);
    }else {
        char *cwd = directory_get_path(sh->vfs_filename, sh->cwd);
        char *mashed = str_prepend(cwd, token);
        path_absolute = path_parse_absolute(sh, mashed);
        free(mashed);
        free(cwd);
    }

    int32_t result = file_delete(sh->vfs_filename, path_absolute);

    if(result == 0){
        printf("OK\n");
    } else{
        printf("FILE NOT FOUND\n");
    }

    free(path_absolute);
}

/**
 * Příkaz: přesun souboru uvnitř VFS
 *
 * @param sh
 * @param command
 */
void cmd_mv(struct shell *sh, char *command){
    if (sh == NULL) {
        log_debug("cmd_mv: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    if (command == NULL) {
        log_debug("cmd_mv: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        return;
    }

    if (strlen(command) < 1) {
        log_debug("cmd_mv: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        return;
    }

    char *first = NULL;
    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");

    // První parametr příkazu
    token = strtok(NULL, " ");
    if(token == NULL){
        printf("mv: First parameter is missing!\n");
        return;
    }
    first = malloc(sizeof(char) * strlen(token) + 1);
    strcpy(first, token);

    // Druhý parametr příkazu
    token = strtok(NULL, " ");
    if(token == NULL){
        free(first);
        printf("mv: Second parameter is missing!\n");
        return;
    }

    // Uprava posledniho parametru - odstraneni \n
    if(token[strlen(token)-1] == '\n'){
        token[strlen(token)-1] = '\0';
    }

    char *path_absolute_source = NULL;


    // Převod na absolutní cestu
    if(starts_with("/", first)){
        path_absolute_source = path_parse_absolute(sh, first);
    }else {
        char *cwd = directory_get_path(sh->vfs_filename, sh->cwd);
        char *mashed = str_prepend(cwd, first);
        path_absolute_source = path_parse_absolute(sh, mashed);
        free(mashed);
        free(cwd);
    }

    if(path_absolute_source == NULL){
        free(first);
        printf("FILE NOT FOUND (neni zdroj)\n");
        log_debug("cmd_mv: Nepodarilo se prevest cestu zdroje na absolutni");
        return;
    }

    char *path_absolute_target = NULL;

    if(strcmp(token, "/") == 0){
        path_absolute_target = malloc(sizeof(char) * 2);
        strcpy(path_absolute_target, "/");
    }
    else {
        // Převod na absolutní cestu
        if (starts_with("/", token)) {
            path_absolute_target = path_parse_absolute(sh, token);
        } else {
            char *cwd = directory_get_path(sh->vfs_filename, sh->cwd);
            char *mashed = str_prepend(cwd, token);
            path_absolute_target = path_parse_absolute(sh, mashed);
            free(mashed);
            free(cwd);
        }
    }

    if(path_absolute_target == NULL){
        free(path_absolute_source);
        free(first);
        printf("PATH NOT FOUND (neexistuje cilova cesta) \n");
        log_debug("cmd_mv: Nepodarilo se prevest cestu cile na absolutni");
        return;
    }

    // Zjištění prefix cesty zdroje
    char *path_prefix = get_prefix_string_until_last_character(path_absolute_source, "/");
    // Zjištění suffix cesty - název souboru
    char *file_name = get_suffix_string_after_last_character(path_absolute_source, "/");

    // Otevření VFS souborů
    VFS_FILE *source_folder = vfs_open(sh->vfs_filename, path_prefix);
    VFS_FILE *target_folder = vfs_open(sh->vfs_filename, path_absolute_target);


    if(source_folder == NULL){
        free(path_absolute_source);
        free(first);
        free(path_absolute_target);
        free(file_name);
        free(path_prefix);
        printf("FILE NOT FOUND (neni zdroj)\n");
        return;
    }

    if(target_folder == NULL){
        vfs_close(source_folder);
        free(path_absolute_source);
        free(first);
        free(path_prefix);
        free(file_name);
        free(path_absolute_target);
        printf("PATH NOT FOUND (neexistuje cílová cesta)\n");
        return;
    }

    log_debug("source_f -> target_f = %d -> %d\n", source_folder->inode_ptr->id, target_folder->inode_ptr->id);

    // Pokud je zdroj a cíl stejný, neděláme nic
    if(source_folder->inode_ptr->id == target_folder->inode_ptr->id){
        vfs_close(source_folder);
        vfs_close(target_folder);
        free(path_absolute_source);
        free(first);
        free(path_prefix);
        free(file_name);
        free(path_absolute_target);

        printf("OK\n");
        return;
    }

    /*
     * Máme zdroj složku a cíl složku, smažeme záznam ze zdroj složky
     * a umístíme ho do cíl složky
     */
    if(directory_has_entry(sh->vfs_filename, source_folder->inode_ptr->id, file_name) < 1){
        vfs_close(source_folder);
        vfs_close(target_folder);
        free(path_absolute_source);
        free(first);
        free(path_prefix);
        free(file_name);
        free(path_absolute_target);

        printf("FILE NOT FOUND (neni zdroj)\n");
        return;
    }

    // Vymazání záznamu v rodiči
    // Alokace dat pro entry
    struct directory_entry *entry = malloc(sizeof(struct directory_entry));

    // Čtení directory entry do velikosti souboru
    int32_t current_index = 0;
    int32_t current = 0;
    while(current + sizeof(struct directory_entry) <= source_folder->inode_ptr->file_size){
        memset(entry, 0, sizeof(struct directory_entry));
        vfs_read(entry, sizeof(struct directory_entry), 1, source_folder);

        if(strcmp(entry->name, file_name) == 0){
            break;
        }

        // Posun na další prvek - vlastně jen pro podmínku, SEEK se upravuje v VFS_READ
        current += sizeof(struct directory_entry);
        current_index = current_index + 1;
    }

    // Kolik má rodič záznamů
    int32_t parent_count = (source_folder->inode_ptr->file_size / sizeof(struct directory_entry));
    int32_t last_parent_entry_index = parent_count - 1;

    /*
     * Je potřeba posunout záznam
     * Posouváme poslední záznam na místo mazaného
     * Zmenšujeme velikost složky o 1 entry
     */
    if(last_parent_entry_index != current_index){
        int64_t seek = sizeof(struct directory_entry) * last_parent_entry_index;
        vfs_seek(source_folder, seek, SEEK_SET);

        // Přečtení poslední entry
        struct directory_entry *replace_entry = malloc(sizeof(struct directory_entry));
        memset(replace_entry, 0, sizeof(struct directory_entry));
        vfs_read(replace_entry, sizeof(struct directory_entry), 1, source_folder);

        // Seek na zápis
        int64_t seek_write = sizeof(struct directory_entry) * current_index;
        vfs_seek(source_folder, seek_write, SEEK_SET);
        vfs_write(replace_entry, sizeof(struct directory_entry), 1, source_folder);

        // Smazání staré entry
        vfs_seek(source_folder, seek, SEEK_SET);
        memset(replace_entry, 0, sizeof(struct directory_entry));
        vfs_write(replace_entry, sizeof(struct directory_entry), 1, source_folder);

        log_debug("directory_delete: Zaznam ve slozce %d presunut na %d\n", last_parent_entry_index, current_index);

        // Zmensen velikosti slozky o smazany zaznam
        source_folder->inode_ptr->file_size = source_folder->inode_ptr->file_size - sizeof(struct directory_entry);
        inode_write_to_index(sh->vfs_filename, source_folder->inode_ptr->id - 1, source_folder->inode_ptr);

        free(replace_entry);
    }

    // Zápis smazaného záznamu do cílové složky
    directory_add_entry(target_folder, entry);

    printf("OK\n");

    // Uvolnění zdrojů
    free(entry);
    vfs_close(source_folder);
    vfs_close(target_folder);
    free(path_absolute_source);
    free(first);
    free(path_prefix);
    free(file_name);
    free(path_absolute_target);
}