#include "debug.h"
#include <string.h>
#include <time.h>


void log_print_stdout(char *level, char *format, va_list args){
    // Kontrola povolení výpisu do terminálu
    if(STDOUT_ENABLED != TRUE) {
        return;
    }

    // Výpis časové značky do terminálu
    if(STDOUT_TIME_ENABLED == TRUE){
        // Deklarace pro čas
        time_t timer;
        char buffer[26];
        struct tm* tm_info;

        // Příprava časové značky
        time(&timer);
        tm_info = localtime(&timer);
        strftime(buffer, 26, LOG_TIME_FORMAT, tm_info);

        // Výpis časové značky
        printf("%s > ", buffer);
    }

    // Zpráva je prázdná, nevypisujeme
    if(strlen(format) < 1) {
        return;
    }

    // Výpis hlavičky
    if(strlen(level) > 1) {
        printf("%s >> ", level);
    }

    // Využití argumentů - výpis zprávy
    vprintf(format, args);
}

void log_print_file(char *level, char *format, va_list args){
    // Kontrola povolení výpisu do souboru
    if(LOG_FILE_ENABLED != TRUE) {
        return;
    }


    // Otevření souboru pro zápis
    FILE *log = fopen(LOG_FILE, LOG_MODE_CURRENT);

    // Kontrola otevření souboru
    if(log == NULL) {
        return;
    }

    // Výpis časové značky do souboru
    if(LOG_FILE_TIME_ENABLED == TRUE){
        // Deklarace pro čas
        time_t timer;
        char buffer[26];
        struct tm* tm_info;

        // Příprava časové značky
        time(&timer);
        tm_info = localtime(&timer);
        strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

        // Výpis časové značky
        fprintf(log, "%s > ", buffer);
    }

    // Výpis hlavičky
    if(strlen(level) > 1) {
        fprintf(log,"%s >> ", level);
    }

    // Využití argumentů - výpis zprávy do souboru
    vfprintf(log,format, args);

    // Uzavření souboru
    fclose(log);
}

/**
 * Wrapper pro funkci printf, výpis při DEBUG=TRUE
 * a DEBUG_LEVEL >= level
 *
 * @param level level výpisu
 * @param format formát textu
 * @param ... parametry formátování
 */
void log_print(int level, char *format, va_list args){
    // Kontrola, zda je logování povoleno
    if(DEBUG == FALSE) {
        return;
    }

    // Kontrola aktuálního levelu
    if(level < DEBUG_LEVEL){
        return;
    }

    // Zpráva je prázdná, nevypisujeme
    if(strlen(format) < 1) {
        return;
    }

    // Výpis hlavičky (čas + level)
    char *level_name;
    switch(level){
        // ALL
        case 0:
            level_name = "";
            break;
            // TRACE
        case 1:
            level_name = "TRACE";
            break;
            // DEBUG
        case 2:
            level_name = "DEBUG";
            break;
            // INFO
        case 3:
            level_name = "INFO";
            break;
            // ERROR
        case 4:
            level_name = "ERROR";
            break;
        case 5:
            level_name = "FATAL";
            break;
        default:
            level_name = "";
    }

    // Kopie argumentů - po prvním využití nejsou argumenty validní
    va_list args_copy;
    va_copy(args_copy, args);
    log_print_file(level_name, format, args_copy);
    va_copy(args_copy, args);
    log_print_stdout(level_name, format, args_copy);
}


/**
 * Wrapper pro funkci printf, výpis pouze při DEBUG=TRUE
 * a DEBUG_LEVEL >= 1
 *
 * @param format formát textu
 * @param ... parametry formátování
 */
void log_trace(char *format, ...){
    // Deklarace argumentů
    va_list args;
    // Naplnění argumentů
    va_start(args, format);
    // Využití argumentů
    log_print(LOG_TRACE, format, args);
    // Ukončení argumentů
    va_end(args);
}


/**
 * Wrapper pro funkci printf, výpis pouze při DEBUG=TRUE
 * a DEBUG_LEVEL >= 2
 *
 * @param format formát textu
 * @param ... parametry formátování
 */
void log_debug(char *format, ...){
        // Deklarace argumentů
        va_list args;
        // Naplnění argumentů
        va_start(args, format);
        // Využití argumentů
        log_print(LOG_DEBUG, format, args);
        // Ukončení argumentů
        va_end(args);
}

/**
 * Wrapper pro funkci printf, výpis pouze při DEBUG=TRUE
 * a DEBUG_LEVEL >= 3
 *
 * @param format formát textu
 * @param ... parametry formátování
 */
void log_info(char *format, ...){
    // Deklarace argumentů
    va_list args;
    // Naplnění argumentů
    va_start(args, format);
    // Využití argumentů
    log_print(LOG_INFO, format, args);
    // Ukončení argumentů
    va_end(args);
}


/**
 * Wrapper pro funkci printf, výpis pouze při DEBUG=TRUE
 * a DEBUG_LEVEL >= 4
 *
 * @param format formát textu
 * @param ... parametry formátování
 */
void log_error(char *format, ...){
    // Deklarace argumentů
    va_list args;
    // Naplnění argumentů
    va_start(args, format);
    // Využití argumentů
    log_print(LOG_ERROR, format, args);
    // Ukončení argumentů
    va_end(args);
}


/**
 * Wrapper pro funkci printf, výpis pouze při DEBUG=TRUE
 * a DEBUG_LEVEL >= 3
 *
 * @param format formát textu
 * @param ... parametry formátování
 */
void log_fatal(char *format, ...){
    // Deklarace argumentů
    va_list args;
    // Naplnění argumentů
    va_start(args, format);
    // Využití argumentů
    log_print(LOG_FATAL, format, args);
    // Ukončení argumentů
    va_end(args);
}