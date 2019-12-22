#ifndef KIV_ZOS_DIRECTORY_H
#define KIV_ZOS_DIRECTORY_H

#include <stdint.h>
#include "vfs_io.h"

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
 * Ověří zda cesta ke složkce existuje, pokud existuje
 * Ověří se, zda složka je prázdná,
 * Pokud složka je prázdná, dealokují se databloky (včetně
 * uvolnění bitmapy a inode index se označí jako volný
 *
 * @param vfs_filename CESTA k VFS souboru
 * @param path cesta uvnitř VFS souboru
 * @return (return < 0: chyba | return = 0: OK | return >0: Message)
 */
int32_t directory_delete(char *vfs_filename, char *path);


/**
 * Vypíše obsah složky ve VFS s danou cestou (příkaz LS)
 * @param vfs_filename cesta k VFS souboru
 * @param path cesta uvnitř VFS
 * @return výsledek operace (return < 0: chyba | return >=0: OK)
 */
int32_t directory_entries_print(char *vfs_filename, char *path);

/**
 * Při nalezení záznamu ve složce vrátí INODE ID záznamu, v opačném případě vrátí
 * hodnotu menší než 1
 *
 * @param vfs_filename cesta k VFS souboru
 * @param inode_id aktualně prohledávaná složka (její inode ID(
 * @param entry_name hledané jméno
 * @return výsledek operace (return < 0: chyba | return==0: Nenalezeno | return >0: Nalezeno)
 */
int32_t directory_has_entry(char *vfs_filename, int32_t inode_id ,char *entry_name);

/**
 * Přidá záznam directory_entry do souboru
 *
 * @param vfs_filename cesta k VFS souboru
 * @param path cesta uvnitř VFS
 * @param entry zapisovaný záznam
 * @return výsledek operace (return < 0: chyba | return >= 0: OK)
 */
int32_t directory_add_entry(VFS_FILE *vfs_parrent, struct directory_entry *entry);


/**
 * Od aktuální inode provede přesun přes záznamy rodičů do root složky
 * a po cestě vytvoří cestu
 *
 * @param vfs_filename
 * @param inode_id
 * @param entry_name
 * @return
 */
char *directory_get_path(char *vfs_filename, int32_t inode_id);

/**
 * Zjistí inode rodičovské složky
 *
 * @param vfs_filename soubor VFS
 * @param inode_id aktuální složka k získání rodiče
 * @return (return < 1: ERR | return > 0: ID)
 */
int32_t directory_get_parent_id(char *vfs_filename, int32_t inode_id);

/**
 * Při nalezení záznamu ve složce vrátí záznam, v opačném případě vrátí NULL
 *
 * @param vfs_filename cesta k VFS souboru
 * @param inode_id aktualně prohledávaná složka (její inode ID(
 * @param entry_name hledané jméno
 * @return výsledek operace (struct directory_entry * | NULL)
 */
struct directory_entry *directory_get_entry(char *vfs_filename, int32_t inode_id ,char *entry_name);




#endif //KIV_ZOS_DIRECTORY_H
