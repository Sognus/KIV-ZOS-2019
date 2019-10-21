#include "debug.h"
#include <string.h>

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

/**
 * Wrapper pro funkci printf, výpis při DEBUG=TRUE
 * a DEBUG_LEVEL >= level
 *
 * @param level level výpisu
 * @param format formát textu
 * @param ... parametry formátování
 */
void log_print(int level, char *format, ...){
    // Kontrola, zda je logování povoleno
    if(DEBUG == FALSE) {
        return;
    }

    // Kontrola aktuálního levelu
    if(level < DEBUG_LEVEL){
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

    // Výpis hlavičky
    if(strlen(level_name) > 1) {
        printf("%s >> ", level_name);
    }

    // Deklarace argumentů
    va_list args;
    // Naplnění argumentů
    va_start(args, format);
    // Využití argumentů - výpis zprávy
    vprintf(format, args);
    // Ukončení argumentů
    va_end(args);
}