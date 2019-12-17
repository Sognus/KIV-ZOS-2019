#include "parsing.h"
#include <stdlib.h>
#include "structure.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "shell.h"
#include "directory.h"

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

/**
 * Porovná dva řetězce, ignoruje velká a malá písmena
 *
 * @param a první řetězec
 * @param b druhá řetězec
 * @return výsledek porovnání
 */
int strcicmp(char const *a, char const *b)
{
    for (;; a++, b++) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d != 0 || !*a) {
            return d;
        }
    }
}

/**
 * Zpracuje řetězec ve tvaru <VELIKOST><JEDNOTKA>
 *
 * @param txt
 */
int64_t parse_filesize(char *txt){
    if(txt == NULL){
        log_debug("parse_filesize: Zpracovavany retezec nesmi byt NULL!\n");
        return -1;
    }

    if(strlen(txt) < 1) {
        log_debug("parse_filesize: Zpracovavany retezec nesmi byt prazdny!\n");
        return -2;
    }

    char *curr = txt;
    int32_t size = 0;

    // Zjištění prvního nečíselného znaku
    while(*curr != '\0'){
        if(isdigit(*curr) == FALSE){
            break;
        }
        size = size + 1;
        curr = curr + sizeof(char);
    }

    // Alokace paměti pro rětězec
    char *unit_str = malloc(sizeof(char) * strlen(txt) + 1);
    char *size_str = malloc(sizeof(char) * strlen(txt) + 1);
    memset(unit_str, 0, sizeof(char) * strlen(txt) + 1);
    memset(size_str, 0, sizeof(char) * strlen(txt) + 1);

    // Kopírování řetězce jednotky do řetězce
    strcpy(unit_str, curr);
    unit_str[strlen(unit_str)-1] = '\0';
    // Úprava původního řetězce před kopírováním velikosti
    txt[size] = '\0';
    // Kopírování řetězce velikosti
    strcpy(size_str, txt);

    int64_t base = 0;
    int64_t multiplicator = 0;

    char *endptr;
    errno = 0;
    int64_t result = strtol(size_str, &endptr, 10);
    bool failed = FALSE;
    if (endptr == size_str)
    {
        failed = TRUE;
    }
    if ((result == LONG_MAX || result == LONG_MIN) && errno == ERANGE)
    {
       failed = TRUE;
    }

    if(failed == FALSE){
        base = result;
    }

    // Vstup je počet byte
    if(strcicmp(unit_str, "B") == 0){
        multiplicator = pow(1024, 0);
    }

    // Vstup je počet kB
    if(strcicmp(unit_str, "kB") == 0){
        multiplicator = pow(1024, 1);
    }

    // Vstup je počet MB
    if(strcicmp(unit_str, "MB") == 0){
        multiplicator = pow(1024, 2);
    }

    // Vstup je počet TB
    if(strcicmp(unit_str, "GB") == 0){
        multiplicator = pow(1024, 3);
    }

    // Vstup je počet TB
    if(strcicmp(unit_str, "TB") == 0){
        multiplicator = pow(1024, 4);
    }

    // Uvolnění zdrojů
    free(unit_str);
    free(size_str);

    // Vrať výsledný počet v byte
    return base * multiplicator;
}

/**
 * Vytvoří v paměti řetězec spojením předpony a řetězce
 * Alokuje pamět
 *
 * @param prefix předpona
 * @param string řetězec
 * @return (char * | NULL)
 */
char *str_prepend(char *prefix, char *string){
    if(string == NULL){
        return NULL;
    }

    if(prefix == NULL) {
        return string;
    }

    char *pref_copy = malloc(sizeof(char) * strlen(prefix) + 1);
    char *str_copy = malloc(sizeof(char) * strlen(string) + 1);
    memset(pref_copy, 0, sizeof(char) * strlen(prefix) + 1);
    memset(str_copy, 0, sizeof(char) * strlen(string) + 1);
    strcpy(pref_copy, prefix);
    strcpy(str_copy, string);

    char *new = malloc(sizeof(char) * strlen(pref_copy) + sizeof(char) * strlen(str_copy) + 1);
    memset(new, 0, sizeof(char) * strlen(pref_copy) + sizeof(char) * strlen(str_copy) + 1);
    strcpy(new, prefix);
    strcat(new, string);

    free(pref_copy);
    free(str_copy);

    return new;
}

/**
 * Zpracuje vstupní řetězec na absolutní cestu
 * PRO EXISTUJÍCÍ CESTY
 *
 * @param sh kontext terminálu
 * @param parsing zpracovávaný řetězec
 * @return (char *cesta | NULL)
 */
char *path_parse_absolute(struct shell *sh, const char *parsing){
    // Pokud je kontext terminalu NULL, nemuzeme pokracovat
    if(sh == NULL){
        log_debug("path_parse_absolute: Kontext terminalu nemuze byt NULL!\n");
        return NULL;
    }

    // Pokud je cesta NULL, nelze vratit
    if(parsing == NULL){
        log_debug("path_parse_absolute: Zpracovavany retezec nemuze byt NULL!\n");
        return NULL;
    }

    // Prazdny retezec nelze zpracovat
    if(strlen(parsing) < 1){
        log_debug("path_parse_absolute: Zpracovavany retezec nemuze byt prazdnym retezcem!\n");
        return NULL;
    }

    char *buffer = malloc(sizeof(char) * strlen(parsing) + 1);
    memset(buffer, 0, sizeof(char) * strlen(parsing) + 1);
    strcpy(buffer, parsing);

    // Ořez řetězce pokud poslední znak je \n
    if(buffer[strlen(buffer)-1] == '\n'){
        buffer[strlen(buffer)-1] = '\0';
    }

    char *part_ptr = buffer;
    char *part = NULL;

    // Pokud řetězec začíná / jedná se o absolutní cestu
    if(starts_with("/", buffer)){
        log_trace("path_parse_absolute: Detekovana absolutni cesta\n");

        // Zjištění prefix cesty
        char *path_prefix = get_prefix_string_until_last_character(buffer, "/");
        // Zjištění suffix cesty - název složky
        char *dir_name = get_suffix_string_after_last_character(buffer, "/");
        // Otevření rodičovské složky
        VFS_FILE *vfs_file = vfs_open_recursive(sh->vfs_filename, path_prefix, 1);
        char *abs = directory_get_path(sh->vfs_filename, vfs_file->inode_ptr->id);

        char *buff = malloc(sizeof(char) * ((strlen(abs) + strlen(dir_name) + 1)));
        memset(buff, 0, sizeof(char) * ((strlen(abs) + strlen(dir_name) + 1)));
        strcpy(buff, abs);
        char *final = str_prepend(abs, dir_name);

        // Uvolnění zdrojů
        vfs_close(vfs_file);
        free(dir_name);
        free(path_prefix);
        free(abs);
        free(buffer);
        free(buff);

        return final;
    }else { // Relativní cesta od shell->cwd
        log_trace("path_parse_absolute: Detekovana relativni cesta\n");

        // Kopie shellu - abychom nezměnili aktuální CWD při parsování
        struct shell *sh_copy = malloc(sizeof(struct shell));
        memset(sh_copy, 0, sizeof(struct shell));
        memcpy(sh_copy, sh, sizeof(struct shell));

        while(1){
            if((part_ptr - (strlen(buffer)+1)) == buffer){
                break;
            }

            part = get_prefix_string_until_first_character(part_ptr, "/");
            part_ptr += strlen(part) +1;

            // Není speciální případ
            if(strcmp(part, "..") != 0 && strcmp(part, ".") != 0){
                struct directory_entry *entry = directory_get_entry(sh_copy->vfs_filename, sh_copy->cwd, part);

                // Část cesty nenalezena
                if(entry == NULL){
                    log_trace("path_parse_absolute: Cast cesty nenalezena: %s z ID=%d\n", sh_copy->cwd, part);
                    free(buffer);
                    free(part);
                    free(sh_copy);
                    return NULL;
                }
                sh_copy->cwd = entry->inode_id;
                free(entry);

            }
            else{ // Speciální případ, přeskočit .
                if(strcmp(part, "..") == 0){
                    // Změna CWD kopie kontextu na rodičovskou složku
                    sh_copy->cwd = directory_get_parent_id(sh_copy->vfs_filename, sh_copy->cwd);
                }
            }

            free(part);
            part = NULL;
        }

        // Vrátit absolutní cestu dle inode
        char *rtn = directory_get_path(sh_copy->vfs_filename, sh_copy->cwd);
        free(sh_copy);
        free(buffer);
        return rtn;

    }

    return NULL;
}

