#ifndef KIV_ZOS_SHELL_H
#define KIV_ZOS_SHELL_H

#include <stdint.h>
#include "bool.h"

#define SHELL_DEFAULT_FOLDER 1

struct shell {
    // ID inode aktuálního pracovního adresáře
    int32_t cwd;
    // Cesta k VFS souboru
    char *vfs_filename;
};

/**
 * Vytvoří strukturu pro kontext terminálu
 *
 * @param vfs_filename
 * @param cwd
 * @return
 */
struct shell *shell_create(char *vfs_filename);

/**
 * Uvolní alokované zdroje pro strukturu shell
 *
 * @param shell_ptr ukazatel na strukturu shell
 * @return výsledek operace
 */
bool shell_free(struct shell *shell_ptr);

/**
 * Zpracuje řetězec obsahující příkaz
 *
 * @param sh ukazatel na kontext shellu
 * @param command příkaz ke zpracování
 */
void shell_parse(struct shell *sh, char *command);

#endif //KIV_ZOS_SHELL_H
