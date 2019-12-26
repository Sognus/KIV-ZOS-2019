#ifndef KIV_ZOS_SYMLINK_H
#define KIV_ZOS_SYMLINK_H

#include <stdint.h>
#include "vfs_io.h"

/**
 * Vytvoří soubor, a uloží do něj cestu na jiný soubor
 * tím vytvoří symlink
 *
 * @param vfs_filename
 * @param path
 * @param linked
 * @return
 */
int32_t symlink_create(char *vfs_filename, char *path, char *linked);


/**
 * Otevře soubor, pokud je soubor SYMLINK vrátí cestu na
 * soubor na který ukazuje
 *
 * @param vfs_filename
 * @param symlink
 * @return
 */
VFS_FILE *symlink_dereference(char *vfs_filename, VFS_FILE *symlink);

#endif //KIV_ZOS_SYMLINK_H
