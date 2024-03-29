#ifndef KIV_ZOS_PARSING_H
#define KIV_ZOS_PARSING_H

/*
 * Hlavičky
 */
#include <string.h>
#include "bool.h"
#include "debug.h"
#include "shell.h"


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

/**
 * Převede typ souboru z čísla na řetězec
 * @param type typ souboru
 * @return ukazatel na řetězec
 */
char *filetype_to_name(int32_t type);

/**
* Ověří, zda je řetězec prefixem druhého
*
* @param prefix prefix
* @param string ověřovaný řetězec
* @return ( TRUE | FALSE )
*/
bool starts_with(const char *prefix, const char *string);

/**
 * Vrátí prefix řetězce do určitého znaku
 *
 * @param string řetězec
 * @param delimiter znak
 * @return char*
 */
char *get_prefix_string_until_first_character(char *string, const char *delimiter);

/**
 * Vrátí suffix řetězce po posledním výskytu určitého znaku
 *
 * @param string řetězec
 * @param delimiter znak
 * @return char*
 */
char *get_suffix_string_after_last_character(char *string, const char *delimiter);

/**
 * Vrátí prefix řetězce do posledního výskytu určitého znaku
 *
 * @param string řetězec
 * @param delimiter znak
 * @return char*
 */
char *get_prefix_string_until_last_character(char *string, const char *delimiter);

/**
 * Porovná dva řetězce, ignoruje velká a malá písmena
 *
 * @param a první řetězec
 * @param b druhá řetězec
 * @return výsledek porovnání
 */
int strcicmp(char const *a, char const *b);

/**
 * Zpracuje řetězec ve tvaru <VELIKOST><JEDNOTKA>
 *
 * @param txt
 */
int64_t parse_filesize(char *txt);

/**
* Vytvoří v paměti řetězec spojením předpony a řetězce
* Alokuje pamět
*
* @param prefix předpona
* @param string řetězec
* @return (char * | NULL)
*/
char *str_prepend(char *prefix, char *string);

/**
 * Zpracuje vstupní řetězec na absolutní cestu
 *
 * @param sh kontext terminálu
 * @param parsing zpracovávaný řetězec
 * @return (char *cesta | NULL)
 */
char *path_parse_absolute(struct shell *sh, const char *parsing);

/**
 * Převede typ souboru z čísla na řetězec
 * @param type typ souboru
 * @return ukazatel na řetězec
 */
char *filetype_to_short(int32_t type);

#endif //KIV_ZOS_PARSING_H
