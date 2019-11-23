#include "parsing.h"
#include <stdlib.h>
#include "structure.h"

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
        log_debug("file_exist: Soubor neexistuje -> ukazatel je NULL");
        return FALSE;
    }

    // Kontrola délky cesty
    if(strlen(path) < 1){
        log_debug("file_exist: Soubor neexistuje -> ukazatel je NULL");
        return FALSE;
    }

    // Pokus o otevření souboru
    FILE *file = fopen(path, "r");
    if (file != NULL)
    {
        fclose(file);
        return TRUE;
    }
    return FALSE;


}

/**
 * Převede typ souboru z čísla na řetězec
 * @param type typ souboru
 * @return ukazatel na řetězec
 */
char *filetype_to_name(int32_t type){
    char *filetype = malloc(sizeof(char) * 32);

    if(type == VFS_DIRECTORY){
        strcpy(filetype, "+DIRECTORY");
        return filetype;
    }

    if(type == VFS_FILE_TYPE) {
        strcpy(filetype, "-FILE");
        return filetype;
    }

    if(type == VFS_SYMLINK) {
        strcpy(filetype, "*SYMLINK");
        return filetype;
    }

    free(filetype);
    return NULL;
}

