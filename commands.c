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
#include "symlink.h"


// Just because Windows is stupid
#ifdef  _WIN32
    #include <errno.h>

    /**
     * Reimplementace pro NON-POSIX systémy (Windows)
     *
     * @param lineptr
     * @param n
     * @param stream
     * @return
     */
    ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
        size_t pos;
        int c;

        if (lineptr == NULL || stream == NULL || n == NULL) {
            errno = EINVAL;
            return -1;
        }

        c = getc(stream);
        if (c == EOF) {
            return -1;
        }

        if (*lineptr == NULL) {
            *lineptr = malloc(128);
            if (*lineptr == NULL) {
                return -1;
            }
            *n = 128;
        }

        pos = 0;
        while(c != EOF) {
            if (pos + 1 >= *n) {
                size_t new_size = *n + (*n >> 2);
                if (new_size < 128) {
                    new_size = 128;
                }
                char *new_ptr = realloc(*lineptr, new_size);
                if (new_ptr == NULL) {
                    return -1;
                }
                *n = new_size;
                *lineptr = new_ptr;
            }

            ((unsigned char *)(*lineptr))[pos ++] = c;
            if (c == '\n') {
                break;
            }
            c = getc(stream);
        }

        (*lineptr)[pos] = '\0';
        return pos;
    }
#endif

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


    // Pokus o otevření souboru
    VFS_FILE *vfs_file = vfs_open(sh->vfs_filename, path_absolute);

    if(vfs_file == NULL){
        printf("PATH NOT FOUND (neexistujici cesta)\n");
        log_debug("cmd_cd: FILE NOT FOUND");
        return;
    }

    // Dereference symlinku
    while(vfs_file->inode_ptr->type == VFS_SYMLINK){
        vfs_file = symlink_dereference(sh->vfs_filename, vfs_file);
    }

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

        if(path_absolute == NULL){
            printf("PATH NOT FOUND (neexistujici adresar)\n");
            return;
        }

        VFS_FILE *vfs_file = vfs_open(sh->vfs_filename, path_absolute);

        if(vfs_file == NULL){
            printf("PATH NOT FOUND (neexistujici adresar)\n");
            free(path_absolute);
            return;
        }

        // Dereference symlinku
        while(vfs_file->inode_ptr->type == VFS_SYMLINK){
            vfs_file = symlink_dereference(sh->vfs_filename, vfs_file);
        }

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
        vfs_close(target);
        printf("FILE NOT FOUND (neni zdroj)\n");
        return;
    }

    // Ziskani superbloku ze souboru
    struct superblock *superblock_ptr = superblock_from_file(sh->vfs_filename);

    if(superblock_ptr == NULL) {
        vfs_close(target);
        fclose(external);
        free(first);
        printf("COULD NOT READ SUPERBLOCK");
        return;
    }


    vfs_seek(target, 0, SEEK_SET);
    fseek(external, 0, SEEK_SET);

    // Vytvoření bufferu o velikosti data bloku
    ssize_t bytes_read = 0;
    ssize_t bytes_written = 0;
    int32_t datablock_size = sizeof(char) * superblock_ptr->cluster_size;
    char *buffer = malloc(datablock_size);
    memset(buffer, 0, datablock_size);

    // Čteme soubor po clusterech VFS
    while (TRUE){
        memset(buffer, 0, datablock_size);
        bytes_read = fread(buffer, sizeof(char), datablock_size, external);

        // Zastavení čtení souboru pokud nemáme data
        if(bytes_read < 1) {
            break;
        }

        size_t write_result = vfs_write(buffer, bytes_read, 1, target);
        if(write_result < 0) {
            printf("PARTIAL WRITE (CODE %zu)\n", write_result);
            vfs_close(target);
            fclose(external);
            free(first);
            free(path_absolute);
            free(superblock_ptr);
            free(buffer);
            return;
        }

        bytes_written += bytes_read;
        printf("Bytes written: %zd\n", bytes_written);
    }

    printf("OK\n");

    // Uvolnění zdrojů
    free(buffer);
    free(path_absolute);
    fclose(external);
    vfs_close(target);
    free(first);
    free(superblock_ptr);
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

    // Dereference symlinku
    while(source->inode_ptr->type == VFS_SYMLINK){
        source = symlink_dereference(sh->vfs_filename, source);
    }

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

    // Smazani presunovaneho zaznamu pokud je posledni
    if(last_parent_entry_index == current_index) {
        // Zmensen velikosti slozky o smazany zaznam
        source_folder->inode_ptr->file_size = source_folder->inode_ptr->file_size - sizeof(struct directory_entry);
        inode_write_to_index(sh->vfs_filename, source_folder->inode_ptr->id - 1, source_folder->inode_ptr);
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

/**
 * Příkaz: kopírování souboru
 *
 * @param sh
 * @param command
 */
void cmd_cp(struct shell *sh, char *command){
    if (sh == NULL) {
        log_debug("cmd_cp: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    if (command == NULL) {
        log_debug("cmd_cp: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        return;
    }

    if (strlen(command) < 1) {
        log_debug("cmd_cp: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        return;
    }

    char *first = NULL;
    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");

    // První parametr příkazu
    token = strtok(NULL, " ");
    if(token == NULL){
        printf("cp: First parameter is missing!\n");
        return;
    }
    first = malloc(sizeof(char) * strlen(token) + 1);
    strcpy(first, token);

    // Druhý parametr příkazu
    token = strtok(NULL, " ");
    if(token == NULL){
        free(first);
        printf("cp: Second parameter is missing!\n");
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
        log_debug("cmd_cp: Nepodarilo se prevest cestu zdroje na absolutni");
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
        log_debug("cmd_cp: Nepodarilo se prevest cestu cile na absolutni");
        return;
    }

    VFS_FILE *source = vfs_open(sh->vfs_filename, path_absolute_source);

    if(source == NULL){
        free(path_absolute_source);
        free(path_absolute_target);
        free(first);
        printf("FILE NOT FOUND (neni zdroj)\n");
        return;
    }

    if(source->inode_ptr->type == VFS_DIRECTORY){
        free(path_absolute_source);
        free(path_absolute_target);
        free(first);
        printf("FILE NOT FOUND (neni zdroj)\n");
    }

    // Vytvoření souboru pokud je potřeba
    int32_t id = file_create(sh->vfs_filename, path_absolute_target);

    VFS_FILE *target = vfs_open(sh->vfs_filename, path_absolute_target);

    if(target == NULL){
        vfs_close(source);
        free(path_absolute_source);
        free(path_absolute_target);
        free(first);
        printf("PATH NOT FOUND (neexistuje cilova cesta) \n");
        log_debug("cmd_incp: Cilove umisteni neexistuje!\n");
        return;
    }

    if(target->inode_ptr->type == VFS_DIRECTORY){
        vfs_close(source);
        free(path_absolute_source);
        free(path_absolute_target);
        free(first);
        printf("PATH NOT FOUND (neexistuje cilova cesta) \n");
        log_debug("cmd_incp: Cilove umisteni neexistuje!\n");
        return;
    }

    vfs_seek(target, 0, SEEK_SET);
    vfs_seek(source, 0, SEEK_SET);

    char *data_buffer = malloc(sizeof(char) * 4096);
    int32_t written = 0;
    while (written <= source->inode_ptr->file_size){
        memset(data_buffer, 0, sizeof(char) * 4096);
        ssize_t read_count = vfs_read(data_buffer, sizeof(char), 4096, source);

        ssize_t write_count = vfs_write(data_buffer, sizeof(char), read_count, target);

        // Posun o počet přečtených byte
        written = written + read_count;

        if(written % 4096 == 0) {
            printf("Copied 4096 bytes (total: %d/%d bytes)\n", written, source->inode_ptr->file_size);
        }
    }

    printf("OK\n");

    // Uvolnění zdrojů
    vfs_close(target);
    vfs_close(source);
    free(path_absolute_source);
    free(path_absolute_target);
    free(first);
    free(data_buffer);
}

/**
 * Příkaz: ;kopie souboru VFS -> system
 *
 * @param sh
 * @param command
 */
void cmd_outcp(struct shell *sh, char *command) {
    if (sh == NULL) {
        log_debug("cmd_outcp: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    if (command == NULL) {
        log_debug("cmd_outcp: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        return;
    }

    if (strlen(command) < 1) {
        log_debug("cmd_outcp: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        return;
    }

    char *first = NULL;
    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");

    // První parametr příkazu
    token = strtok(NULL, " ");
    if(token == NULL){
        printf("outcp: First parameter is missing!\n");
        return;
    }
    first = malloc(sizeof(char) * strlen(token) + 1);
    strcpy(first, token);

    // Druhý parametr příkazu
    token = strtok(NULL, " ");
    if(token == NULL){
        free(first);
        printf("outcp: Second parameter is missing!\n");
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
        log_debug("cmd_outcp: Nepodarilo se prevest cestu zdroje na absolutni");
        return;
    }

    VFS_FILE *source = vfs_open(sh->vfs_filename, path_absolute_source);

    if(source == NULL){
        free(first);
        free(path_absolute_source);
        printf("FILE NOT FOUND (neni zdroj)\n");
        return;
    }

    FILE *target = fopen(token, "wb");

    if(target == NULL){
        printf("PATH NOT FOUND (neexistuje cilova cesta)\n");
        vfs_close(source);
        free(first);
        free(path_absolute_source);
        return;
    }

    vfs_seek(source, 0, SEEK_SET);
    fseek(target, 0, SEEK_SET);

    char *buffer = malloc(sizeof(char) * 4096);
    int64_t written = 0;
    while(written < source->inode_ptr->file_size){
        memset(buffer, 0, sizeof(char) * 4096);
        ssize_t vfs_read_count = vfs_read(buffer, sizeof(char), 4096, source);

        if(vfs_read_count < 0){
            printf("PARTIAL WRITE!\n");
            vfs_close(source);
            fclose(target);
            free(buffer);
            free(first);
            free(path_absolute_source);
            return;
        }

        size_t written_count = fwrite(buffer, sizeof(char), vfs_read_count, target);

        // Dosažení konce souboru
        if(vfs_read_count < 1) {
            break;
        }

        written = written + written_count;
        if(written_count % 4096 == 0) {
            printf("Written out: 4096B (total: %d bytes)\n", written);
        }
    }

    printf("OK\n");

    vfs_close(source);
    fclose(target);
    free(buffer);
    free(first);
    free(path_absolute_source);
}

/**
 * Příkaz: načtení příkazů ze souboru
 *
 * @param sh
 * @param command
 */
void cmd_load(struct shell *sh, char *command){
    if (sh == NULL) {
        log_debug("cmd_load: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    if (command == NULL) {
        log_debug("cmd_load: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        return;
    }

    if (strlen(command) < 1) {
        log_debug("cmd_load: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        return;
    }

    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");

    // První parametr příkazu
    token = strtok(NULL, " ");
    if(token == NULL){
        printf("load: Parameter is missing!\n");
        return;
    }

    // Uprava posledniho parametru - odstraneni \n
    if(token[strlen(token)-1] == '\n'){
        token[strlen(token)-1] = '\0';
    }

    if(file_exist(token) != TRUE){
        printf("FILE NOT FOUND (neni zdroj)\n");
        return;
    }

    FILE *source = fopen(token, "r+b");

    if(source == NULL){
        printf("FILE NOT FOUND (neni zdroj)\n");
        return;
    }

    size_t len = 0;
    char *line = NULL;
    ssize_t read;
    while ((read = getline(&line, &len, source)) != -1) {
        char *line_copy = malloc(sizeof(char) * strlen(line) + 1);
        memset(line_copy, 0, sizeof(char) * strlen(line) + 1);
        strcpy(line_copy, line);

        // odstraneni \n
        if(line_copy[strlen(line_copy)-1] == '\n'){
            line_copy[strlen(line_copy)-1]  = '\0';
        }

        // Výpis aktuálního příkazu
        printf("%s -> ", line_copy);

        // Uvolnění zdroje
        free(line_copy);

        // Zpracování příkazu
        shell_parse(sh, line);
    }


    printf("OK\n");


}

/**
 * Příkaz: Výpis informací o souboru
 *
 * @param sh
 * @param command
 */
void cmd_info(struct shell *sh, char *command){
    if (sh == NULL) {
        printf("FILE NOT FOUND (neni zdroj)\n");
        log_debug("cmd_info: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    if (command == NULL) {
        printf("FILE NOT FOUND (neni zdroj)\n");
        log_debug("cmd_info: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        return;
    }

    if (strlen(command) < 1) {
        printf("FILE NOT FOUND (neni zdroj)\n");
        log_debug("cmd_info: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        return;
    }

    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");

    // První parametr příkazu
    token = strtok(NULL, " ");
    if(token == NULL){
        printf("info: Parameter is missing!\n");
        return;
    }

    // Uprava posledniho parametru - odstraneni \n
    if(token[strlen(token)-1] == '\n'){
        token[strlen(token)-1] = '\0';
    }

    char *path_absolute_source = NULL;

    if(strcmp(token, "/") == 0){
        path_absolute_source = malloc(sizeof(char) * 2);
        memset(path_absolute_source, 0, sizeof(char) * 2);
        strcpy(path_absolute_source, "/");
    }
    else {
        // Převod na absolutní cestu
        if (starts_with("/", token)) {
            path_absolute_source = path_parse_absolute(sh, token);
        } else {
            char *cwd = directory_get_path(sh->vfs_filename, sh->cwd);
            char *mashed = str_prepend(cwd, token);
            path_absolute_source = path_parse_absolute(sh, mashed);
            free(mashed);
            free(cwd);
        }
    }

    VFS_FILE *source = vfs_open(sh->vfs_filename, path_absolute_source);

    if(source == NULL){
        free(token);
        free(path_absolute_source);
        printf("FILE NOT FOUND (neni zdroj)\n");
        return;
    }

    char *vfs_name = NULL;

    if(strcmp(path_absolute_source, "/") == 0){
        vfs_name = malloc(sizeof(char) * 2);
        memset(vfs_name, 0, sizeof(char) * 2);
        strcpy(vfs_name, "/");
    } else {
        vfs_name = get_suffix_string_after_last_character(token, "/");
    }

    // Výpis názvu
    printf("NAME: %s\n", vfs_name);
    // Výpis INODE-ID
    printf("INODE-ID: %d\n", source->inode_ptr->id);
    // Výpis TYPE
    char *filetype = filetype_to_name(source->inode_ptr->type);
    printf("TYPE: %s\n", filetype);
    free(filetype);
    // Výpis velikosti souboru
    printf("SIZE: %d (byte/s)\n", source->inode_ptr->file_size);

    // Výpis datových odkazů
    printf("DATA POINTERS: \n");

    if(source->inode_ptr->direct1 != 0){
        printf("\tdirect1: 0x%x\n", source->inode_ptr->direct1);
    }

    if(source->inode_ptr->direct2 != 0){
        printf("\tdirect2: 0x%x\n", source->inode_ptr->direct2);
    }

    if(source->inode_ptr->direct3 != 0){
        printf("\tdirect3: 0x%x\n", source->inode_ptr->direct3);
    }

    if(source->inode_ptr->direct4 != 0){
        printf("\tdirect4: 0x%x\n", source->inode_ptr->direct4);
    }

    if(source->inode_ptr->direct5 != 0){
        printf("\tdirect5: 0x%x\n", source->inode_ptr->direct5);
    }

    struct superblock *superblock_ptr = superblock_from_file(sh->vfs_filename);

    // Indirect 1
    if(source->inode_ptr->indirect1 != 0){
        printf("indirect1: 0x%x\n", source->inode_ptr->indirect1);

        FILE *data = fopen(sh->vfs_filename, "r+b");
        fseek(data, source->inode_ptr->indirect1, SEEK_SET);

        int32_t *read_data = malloc(sizeof(int32_t));

        int32_t indirect1_read = 0;
        int32_t adress_per_cluster = superblock_ptr->cluster_size / sizeof(int32_t);
        while(indirect1_read < adress_per_cluster){
            memset(read_data, 0, sizeof(int32_t));
            fread(read_data, sizeof(int32_t), 1, data);

            if(*read_data == 0){
                break;
            }

            printf("\t\tindirect1[%d]: 0x%x\n", indirect1_read, *read_data);

            indirect1_read = indirect1_read + 1;
        }

        free(read_data);
        fclose(data);
    }

    // Indirect 2
    if(source->inode_ptr->indirect2 != 0){
        printf("indirect2: 0x%x\n", source->inode_ptr->indirect2);

        FILE *data = fopen(sh->vfs_filename, "r+b");
        fseek(data, source->inode_ptr->indirect2, SEEK_SET);

        int32_t adress_per_cluster = superblock_ptr->cluster_size / sizeof(int32_t);

        int32_t *read_data = malloc(sizeof(int32_t));

        int32_t indirect2_level1_read = 0;
        while(indirect2_level1_read < adress_per_cluster){
            fseek(data, source->inode_ptr->indirect2 + sizeof(int32_t) * indirect2_level1_read, SEEK_SET);
            memset(read_data, 0, sizeof(int32_t));
            fread(read_data, sizeof(int32_t), 1, data);

            if(*read_data == 0){
                break;
            }

            printf("\tindirect2[%d]: 0x%x\n", indirect2_level1_read, *read_data);

            int32_t *level2_read_data = malloc(sizeof(int32_t));

            int32_t indirect2_level2_read = 0;
            while(indirect2_level2_read < adress_per_cluster){
                fseek(data, *read_data + sizeof(int32_t) * indirect2_level2_read, SEEK_SET);
                memset(level2_read_data, 0, sizeof(int32_t));
                fread(level2_read_data, sizeof(int32_t), 1, data);

                if(*level2_read_data == 0) {
                    break;
                }

                printf("\t\tindirect2[%d][%d]: 0x%x\n", indirect2_level1_read, indirect2_level2_read, *level2_read_data);

                indirect2_level2_read = indirect2_level2_read + 1;
            }

            free(level2_read_data);


            indirect2_level1_read++;
        }

        free(read_data);
        fclose(data);
    }

    vfs_close(source);
    free(vfs_name);
    free(path_absolute_source);
}

/**
 * Příkaz: vytvoření symlinku
 *
 * @param sh
 * @param command
 */
void cmd_lns(struct shell *sh, char *command){
    if (sh == NULL) {
        log_debug("cmd_lns: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    if (command == NULL) {
        log_debug("cmd_lns: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        return;
    }

    if (strlen(command) < 1) {
        log_debug("cmd_lns: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
        return;
    }

    char *first = NULL;
    char *token = NULL;
    // Jméno příkazu
    token = strtok(command, " ");

    // První parametr příkazu
    token = strtok(NULL, " ");
    if(token == NULL){
        printf("lns: First parameter is missing!\n");
        return;
    }
    first = malloc(sizeof(char) * strlen(token) + 1);
    strcpy(first, token);

    // Druhý parametr příkazu
    token = strtok(NULL, " ");
    if(token == NULL){
        free(first);
        printf("lns: Second parameter is missing!\n");
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
        printf("FILE NOT FOUND (neexistuje linkovany soubor)\n");
        log_debug("cmd_lns: Nepodarilo se prevest cestu zdroje na absolutni");
        return;
    }

    // Ověření existence zdrojového souboru
    VFS_FILE *source = vfs_open(sh->vfs_filename, path_absolute_source);

    if(source == NULL){
        free(first);
        free(path_absolute_source);
        printf("FILE NOT FOUND (neexistuje linkovany soubor)\n");
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
        vfs_close(source);
        free(path_absolute_source);
        free(first);
        printf("PATH NOT FOUND (nelze vytvorit symlink) \n");
        log_debug("cmd_cp: Nepodarilo se prevest cestu cile na absolutni");
        return;
    }

    int32_t symlink_result = symlink_create(sh->vfs_filename, path_absolute_target, path_absolute_source);

    if(symlink_result != 0){
        vfs_close(source);
        free(path_absolute_source);
        free(first);
        printf("PATH NOT FOUND (nelze vytvorit symlink)\n");
        return;
    }

    printf("OK\n");
}