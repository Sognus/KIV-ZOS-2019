#ifndef KIV_ZOS_INODE_H
#define KIV_ZOS_INODE_H

/*
 * Nutné hlavičky
 */
#include <stdint.h>
#include "bool.h"


/*
 * Konstanty
 */
#define ID_ITEM_FREE 0

/*
 * Struktury
 */
struct inode {
    int32_t id;                 // ID i-uzlu; pokud ID == ID_ITEM_FREE, je položka volná
    int8_t type;                // Typ i-uzlu; 0 = soubor; 1 = složka; 2 = symlink
    int8_t references;          // Počet odkazů na i-uzel; používá se pro hardlinky
    int32_t allocated_clusters; // Počet alokovavaných clusterů (počet odkazů na datové bloky)
    int32_t file_size;          // Velikost souboru v bytech
    int32_t direct1;            // 1. přímý odkaz na datové bloky
    int32_t direct2;            // 2. přímý odkaz na datové bloky
    int32_t direct3;            // 3. přímý odkaz na datové bloky
    int32_t direct4;            // 4. přímý odkaz na datové bloky
    int32_t direct5;            // 5. přímý odkaz na datové bloky
    int32_t indirect1;          // 1. nepřímý odkaz (odkaz - datové bloky)
    int32_t indirect2;          // 2. nepřímý odkaz (odkaz - odkaz - datové bloky)
};

/**
 * Vypíše obsah struktury inode
 *
 * @param ptr ukazatel na strukturu inode
 */
void inode_print(struct inode *ptr);


/**
 * Vrátí první volný index pro inode
 *
 * @param filename
 * @return
 */
int32_t inode_find_free_index(char *filename);

/**
 * Na základě indexu vrátí adresu inode
 *
 * @param filename soubor vfs
 * @param inode_index index inode
 * @return
 */
int32_t inode_index_to_adress(char *filename, int32_t inode_index);

/**
 * Zapíše obsah struktury inode na adresu ve VFS určenou indexem
 *
 * @param filename soubor vfs
 * @param inode_index index inode ve VFS
 * @param inode_ptr struktura k zapsání
 * @return výsledek operace
 */
int32_t inode_write_to_index(char *filename, int32_t inode_index, struct inode *inode_ptr);

/**
 * Zapíše obsah struktury inode na adresu ve VFS
 *
 * @param filename soubor vfs
 * @param inode_address adresa inode ve VFS
 * @param inode_ptr struktura k zapsání
 * @return výsledek operace
 */
int32_t inode_write_to_address(char *filename, int32_t inode_address, struct inode *inode_ptr);

/**
 * Pokusí se o přečtení struktury inode z VFS a vrátí ukazatel
 * hledá inode dle indexu
 *
 * @param filename soubor VFS
 * @param inode_index index inode
 * @return výsledek operace (PTR | NULL)
 */
struct inode *inode_read_by_index(char *filename, int32_t inode_index);


/**
 * Pokusí se o přečtení struktury inode z VFS a vrátí ukazatel
 * hledá inode dle indexu
 *
 * @param filename soubor VFS
 * @param inode_address adresa inode ve VFS
 * @return výsledek operace (PTR | NULL)
 */
struct inode *inode_read_by_address(char *filename, int32_t inode_address);

/**
 * Kontrolní funkce, která ověří, zda lze převést adresu na index data bloku
 *
 * @param filename soubor vfs
 * @param address adresa ve VFS
 * @return výsledek operace (return < 0 - chyba | return >= 0 - validní index)
 */
int32_t inode_data_index_from_address(char *filename, int32_t address);

/**
 * Přidá nový ukazatel na datový blok pro strukturu
 *
 * @deprecated Prochází lineárně všechny existující odkazy a hledá volný odkaz - pomalé
 *
 * @param filename soubor VFS
 * @param inode_ptr ukazatel na pozměňovaný inode
 * @return výsledek operace
 */
bool inode_add_data_address_slow(char *filename, struct inode *inode_ptr, int32_t address);


/**
 * Přidá nový ukazatel na datový blok pro strukturu - rychlejší verze
 *
 * @param filename soubor VFS
 * @param inode_ptr ukazatel na pozměňovaný inode
 * @return výsledek operace
 */
bool inode_add_data_address(char *filename, struct inode *inode_ptr, int32_t address);

#endif //KIV_ZOS_INODE_H
