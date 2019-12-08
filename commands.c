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
 * Příkaz FORMAT
 *
 * @param sh kontext virtuálního terminálu
 * @param command příkaz
 */
void cmd_format(struct shell *sh, char *command) {
    if(sh == NULL){
        log_debug("cmd_format: Nelze zpracovat prikaz. Kontext terminalu je NULL!\n");
        return;
    }

    if(command == NULL){
        log_debug("cmd_format: Nelze zpracovat prikaz. Prikaz je NULL!\n");
        return;
    }

    if(strlen(command) < 1){
        log_debug( "cmd_format: Nelze zpracovat prikaz. Prikaz je prazdnym retezcem!\n");
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
    }
}
