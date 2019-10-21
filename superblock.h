#ifndef KIV_ZOS_SUPER_H
#define KIV_ZOS_SUPER_H

/*
 * Vyhrazené místo na:
 *      * superblock
 *      * bitmapu datových bloků
 *      * i-uzly
 * je 4% z celkové velikosti VFS
 */

/*
 * Nutné hlavičky
 */
#include <stdint.h>
#include "debug.h"
#include "bool.h"

/*
 * Konstanty
 */
// None

/*
 * Struktury
 */
struct superblock {
    char signature[9];                  // Login autora FS
    char volume_descriptor[251];        // Popis vygenerovaného FS
    int32_t disk_size;                  // Celková velikost FS
    int32_t cluster_size;               // Velikost clusterů
    int32_t cluster_count;              // Počet clusterů
    int32_t bitmap_start_address;       // Adresa počátku bitmapy datových bloků
    int32_t inode_start_address;        // Adresa počátku i-uzlů
    int32_t data_start_address;         // Adresa počátku datových bloků
};

/**
 * Alokuje strukturu superblok a vyplní ji implicitními daty
 *
 *
 *
 * @param disk_size celková velikost VFS
 * @return ukazatel na superblock
 */
struct superblock* superblock_impl_alloc(int32_t disk_size);

/**
 * Kompletně alokuje a vyplní strukturu superblock
 *
 * @param disk_size celková velikost VFS
 * @param signature signature superbloku
 * @param volume_descriptor volume_descriptor superbloku
 * @return ukazatel na superblock
 */
struct superblock* superblock_alloc(int32_t disk_size, int32_t cluster_size, char signature[9], char volume_descriptor[251]);

/**
 * Nastaví signature struktury superblock na novou hodnotu
 *
 * @param ptr ukazatel na strukturu superblock
 * @param signature nová hodnota signature
 * @return úspěch operace 0/1
 */
bool superblock_set_signature(struct superblock *ptr, char *signature);

/**
 * Nastaví hodnotu volume_descriptor struktury superblock na novou hodnotu
 *
 * @param ptr ukazatel na strukturu superblock
 * @param signature nová hodnota volume_descriptor
 * @return úspěch operace 0/1
 */
bool superblock_set_volume_descriptor(struct superblock *ptr, char *volume_descriptor);

/**
 * Ověří obsah struktury superblock
 *
 * @param ptr ukazatel na strukturu superblock
 * @return validita superblocku (0 | 1)
 */
bool superblock_check(struct superblock *ptr);

/**
 * Vypíše obsah struktury superblock
 *
 * @param ptr ukazatel na strukturu
 */
void superblock_print(struct superblock *ptr);

#endif //KIV_ZOS_SUPER_H
