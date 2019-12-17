#ifndef KIV_ZOS_FILE_H
#define KIV_ZOS_FILE_H

#include <stdint.h>
#include "parsing.h"

/**
 * Vytvoří soubor ve VFS
 *
 * @param vfs_filename cesta k VFS
 * @param path cesta k souboru
 * @return
 */
int32_t file_create(char *vfs_filename, char *path);

#endif //KIV_ZOS_FILE_H
