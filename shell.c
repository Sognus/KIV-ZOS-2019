#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "superblock.h"
#include "inode.h"
#include "parsing.h"
#include "commands.h"
#include "shell.h"



/**
 * Vytvoří strukturu pro kontext terminálu
 *
 * @param vfs_filename
 * @param cwd
 * @return
 */
struct shell *shell_create(char *vfs_filename){
    // Ověření - NOT NULL
    if (vfs_filename == NULL) {
        log_debug("shell_create: Parametr vfs_filename nemuze byt NULL\n0");
        return NULL;
    }

    // Ověření - délka řetězce
    if(strlen(vfs_filename) < 1){
        log_debug("shell_create: Parametr vfs_filename nemuze byt prazdny retezec!\n");
        return NULL;
    }

    // Pokus o přečtení zda existuje superblock
    struct superblock *superblock_ptr = superblock_from_file(vfs_filename);

    // Superblock not NULL
    if(superblock_ptr == NULL){
        log_debug("shell_create: Nepodarilo se ziskat superblock ze souboru!\n");
        return NULL;
    }

    // Ověření konzistence superbloku
    if(superblock_check(superblock_ptr) != TRUE){
        log_debug("shell_create: Superblok v souboru %s neni validni!\n", vfs_filename);
        free(superblock_ptr);
        return NULL;
    }

    // Pouze informační: Ověření existence kořenové složky - ID=1 -> index=0
    struct inode *inode_ptr = inode_read_by_index(vfs_filename, 0);
    int32_t cwd = 0;

    if(inode_ptr == NULL){
        log_debug("shell_create: Nelze nacist informace o korenove slozce z %s\n", vfs_filename);
    } else {
        cwd = inode_ptr->id;
    }

    // Alokace kontextu
    struct shell *shell_ptr = malloc(sizeof(struct shell));
    memset(shell_ptr, 0, sizeof(struct shell));

    shell_ptr->vfs_filename = malloc(sizeof(char) * strlen(vfs_filename) + 1);
    shell_ptr->cwd = cwd;
    strcpy(shell_ptr->vfs_filename, vfs_filename);

    // Uvolnění zdrojů
    free(superblock_ptr);
    free(inode_ptr);

    return shell_ptr;
}

/**
 * Uvolní alokované zdroje pro strukturu shell
 *
 * @param shell_ptr ukazatel na strukturu shell
 * @return výsledek operace
 */
bool shell_free(struct shell *shell_ptr){
    if(shell_ptr == NULL){
        return FALSE;
    }

    if(shell_ptr->vfs_filename != NULL){
        free(shell_ptr->vfs_filename);
    }

    free(shell_ptr);

    return TRUE;
}


/**
 * Zpracuje řetězec obsahující příkaz
 *
 * @param sh ukazatel na kontext shellu
 * @param command příkaz ke zpracování
 */
void shell_parse(struct shell *sh, char *command){
    // Kontrola zda kontext terminalu neni NULL
    if(sh == NULL){
        log_fatal("shell_parse: Nelze zpracovat prikaz. Kontext terminalu nemuze byt NULL!\n");
        return;
    }

    // Kontrola zda ukazatel na řetězec není NULL
    if(command == NULL){
        log_fatal("shell_parse: Nelze zpracovat prikaz. Prikaz ke zpracovani nemuze byt NULL!\n");
        return;
    }

    // Kontrola délky řetězce
    if(strlen(command) < 1){
        log_fatal("shell_parse: Nelze zpracovat prikaz. Prikaz ke zpracovani nemuze byt prazdnym retezcem!\n");
        return;
    }

    char *cmd = malloc(sizeof(char) * strlen(command) + 1);
    memset(cmd, 0, sizeof(char) * strlen(command) + 1);
    strcpy(cmd, command);

    char *token = NULL;
    token = strtok(command, " ");

    if(strcicmp(token, "format\n") == 0){
        printf("format: Required parameter is missing!\n");
    }

    // Příkaz -> formátování VFS
    if(strcicmp(token, "format") == 0){
        cmd_format(sh, cmd);
    }

    // Příkaz -> výpis aktuální cesty
    if(strcicmp(token, "pwd\n") == 0) {
        cmd_pwd(sh);
    }

    // Příkaz -> změna adresáře -> chybějící parametry
    if(strcicmp(token, "cd\n") == 0){
        printf("cd: Required parameter is missing!\n");
    }

    // Příkaz -> změna adresáře
    if(strcicmp(token, "cd") == 0){
        cmd_cd(sh, cmd);
    }

    // Příkaz -> vytvoření adresáře -> chybějící parametry
    if(strcicmp(token, "mkdir\n") == 0){
        printf("mkdir: Required parameter is missing!\n");
    }

    // Příkaz -> vytvoření adresáře
    if(strcicmp(token, "mkdir") == 0){
        cmd_mkdir(sh, cmd);
    }

    // Příkaz -> ls -> bez parametrů => ls $cwd
    if(strcicmp(token, "ls\n") == 0){
        cmd_ls(sh, NULL);
    }

    // Příkaz -> ls -> parametrický
    if(strcicmp(token, "ls") == 0){
        printf("ls: Not yet implemented!\n");
    }


    // Uvolnění zdrojů
    free(cmd);



}