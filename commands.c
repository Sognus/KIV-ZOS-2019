#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "shell.h"
#include "debug.h"
#include "parsing.h"
#include "commands.h"
#include "structure.h"
#include "directory.h"

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
        printf("FILE NOT FOUND\n");
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
        printf("FILE NOT FOUND\n");
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
        // TODO: implement
    }


}