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

/**
 * Ověří, zda je řetězec prefixem druhého
 *
 * @param prefix prefix
 * @param string ověřovaný řetězec
 * @return ( TRUE | FALSE )
 */
bool starts_with(const char *prefix, const char *string)
{
    return strncmp(prefix, string, strlen(prefix)) == 0;
}

/**
 * Vrátí prefix řetězce do prvního výskytu určitého znaku
 *
 * @param string řetězec
 * @param delimiter znak
 * @return char*
 */
char *get_prefix_string_until_first_character(char *string, const char *delimiter){
    char *token_ptr;

    // Vytvoření bufferu protože strtok je p**a
    char *buffer = malloc(sizeof(char)*strlen(string)+1);
    memset(buffer, 0, strlen(string)+1);
    strcpy(buffer, string);

    // Získání první části
    token_ptr = strtok(buffer, "/" );

    // Buffer 2
    char *buffer2 = malloc(sizeof(char)*strlen(string)+1);
    memset(buffer2, 0, strlen(string)+1);
    strcpy(buffer2, token_ptr);

    free(buffer);

    // Návrat první části
    return buffer2;
}

/**
 * Vrátí prefix řetězce do posledního výskytu určitého znaku
 *
 * @param string řetězec
 * @param delimiter znak
 * @return char*
 */
char *get_prefix_string_until_last_character(char *string, const char *delimiter){
    char *token_ptr;

    // Vytvoření bufferu protože strtok je p**a
    char *buffer = malloc(sizeof(char)*strlen(string)+1);
    memset(buffer, 0, strlen(string)+1);
    strcpy(buffer, string);

    // Získání části za znakem
    token_ptr = get_suffix_string_after_last_character(string, delimiter);
    // Místo useknutí
    int32_t offset = strlen(string) - strlen(token_ptr);
    int32_t size = strlen(token_ptr);
    // Useknutí
    memset(buffer + offset, 0, size);
    free(token_ptr);

    // Návrat první části
    return buffer;
}

/**
 * Vrátí suffix řetězce po posledním výskytu určitého znaku
 *
 * @param string řetězec
 * @param delimiter znak
 * @return char*
 */
char *get_suffix_string_after_last_character(char *string, const char *delimiter){
    char *token_ptr;
    char *prev_token_ptr = NULL;

    // Vytvoření bufferu protože strtok je p**a
    char *buffer = malloc(sizeof(char)*strlen(string)+1);
    memset(buffer, 0, strlen(string)+1);
    strcpy(buffer, string);

    // Získání první části
    token_ptr = strtok(buffer, "/" );

    /* walk through other tokens */
    while( token_ptr != NULL ) {
        prev_token_ptr = token_ptr;
        token_ptr = strtok(NULL, delimiter);
    }

    char *buffer2 = malloc(sizeof(char)*strlen(string)+1);
    memset(buffer2, 0, strlen(string)+1);
    strcpy(buffer2, prev_token_ptr);
    free(buffer);

    // Návrat první části
    return buffer2;
}

