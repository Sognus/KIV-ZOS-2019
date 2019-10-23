#ifndef KIV_ZOS_BITMAP_H
#define KIV_ZOS_BITMAP_H

/*
 * Hlavičky
 */
#include <stdint.h>
#include "bool.h"

/*
 * Konstanty
 */

/*
 * Struktury
 */

/**
 * Vypíše řádkovou reprezentaci bitmapy
 *
 * @param filename soubor VFS
 */
int32_t bitmap_print(char *filename);

/**
 * Nastaví souvislý blok hodnot v bitmapě na zvolenou hodnotu,
 * funkce neřeší kolizi již existujících dat.
 *
 * @param filename soubor vfs
 * @param index počáteční index zápisu
 * @param count počet členů v bloku
 * @param value hodnota
 * @return výsledek operace (return < 0 - chyby  | 0 - úspěch | return > 0 - kolik zápisů se nepodařilo)
 */
int32_t bitmap_set(char *filename, int32_t index, int32_t count, bool value);

/**
 * Vrátí hodnotu bitmapy na určeném indexu
 *
 * @param filename soubor VFS
 * @param index index dat
 * @return hodnota dat na indexu
 */
bool bitmap_get(char *filename, int32_t index);


/**
 * Na základě indexu vypočte počáteční adresu clusteru
 *
 * @param filename soubor VFS
 * @param index index clusteru
 * @return (return < 0 - chyba | return > 0 - adresa clusteru ve VFS)
 */
int32_t bitmap_index_to_cluster_address(char *filename, int32_t index);

/**
 * Vrátí první volný cluster v bitmapě
 *
 * @param filename soubor vfs
 * @return  index volného clusteru
 */
int32_t bitmap_find_free_cluster_index(char *filename);

#endif //KIV_ZOS_BITMAP_H
