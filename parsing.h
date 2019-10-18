#ifndef KIV_ZOS_PARSING_H
#define KIV_ZOS_PARSING_H

/*
 * Hlavičky
 */
#include <string.h>
#include "bool.h"
#include "debug.h"


/*
 * Konstanty
 */
// None


/*
 * Struktury
 */
// None

/**
 * Zkontroluje zda je možné umocnit číslo 2 tak, abychom
 * dosáhli stejné hodnoty jako parametr.
 *
 *
 * @param number kontrolované číslo
 * @return vyhovuje číslo podmínce? (1 / 0)
 */
bool is_two_power(int32_t number);

/**
 * Pokusí se otevřít soubor ke čtení a tím ověří jeho existenci
 *
 * @param path cesta k souboru
 * @return existence souboru (0 | 1)
 */
bool file_exist(char *path);

#endif //KIV_ZOS_PARSING_H
