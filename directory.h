#ifndef KIV_ZOS_DIRECTORY_H
#define KIV_ZOS_DIRECTORY_H

#include <stdint.h>

/*
 * Struktura DIRECTORY ve VFS
 *      directory_entry - 16byte
 *      počet záznamů - CLUSTER_SIZE / 16 (např. 4096/16 = 256)
 *      pevné záznamy:
 *          .   aktuální soubor
 *         ..   rodič (u root - . a .. stejné)
 *         [254 dalších záznamů]
 */

// Strukturovaný záznam souboru ve složce
struct directory_entry {
    char name[12];          // Jméno souboru -> 0-7 jméno, 8-10 - přípona, 11 - \0
    int32_t inode_id;       // Index inode
};

/**
 * Vytvoří ve VFS novou složku
 *
 * @param vfs_filename cesta k VFS souboru
 * @param path cesta uvnitř VFS
 * @return výsledek operace (return < 0: chyba | return >=0: OK)
 */
int32_t directory_create(char *vfs_filename, char *path);

/**
 * Vypíše obsah složky ve VFS s danou cestou (příkaz LS)
 * @param vfs_filename cesta k VFS souboru
 * @param path cesta uvnitř VFS
 * @return výsledek operace (return < 0: chyba | return >=0: OK)
 */
int32_t directory_entries_print(char *vfs_filename, char *path);


#endif //KIV_ZOS_DIRECTORY_H
