#include "parsing.h"

/**
 * Zkontroluje zda je možné umocnit číslo 2 tak, abychom
 * dosáhli stejné hodnoty jako parametr.
 *
 *
 * @param number kontrolované číslo
 * @return vyhovuje číslo podmínce? (1 / 0)
 */

bool is_two_power(int32_t number){
    // Počáteční hodnota 2^0 = 1
    int32_t result = 1;

    while(result < number)
    {
        result = result * 2;
        if(result == number){
            return TRUE;
        }
    }
    // Číslo není mocninou 2
    return FALSE;
}

/**
 * Pokusí se otevřít soubor ke čtení a tím ověří jeho existenci
 *
 * @param path cesta k souboru
 * @return existence souboru (0 | 1)
 */
bool file_exist(char *path){
    // Kontrola ukazatele
    if(path == NULL){
        debug_print("file_exist: Soubor neexistuje -> ukazatel je NULL");
        return FALSE;
    }

    // Kontrola délky cesty
    if(strlen(path) < 1){
        debug_print("file_exist: Soubor neexistuje -> ukazatel je NULL");
        return FALSE;
    }

    // Pokus o otevření souboru
    FILE *file;
    if ((file = fopen(path, "r")))
    {
        fclose(file);
        return TRUE;
    }
    return FALSE;


}

