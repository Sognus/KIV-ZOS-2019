#include "debug.h"

/**
 * Wrapper pro funkci printf, výpis pouze při DEBUG=TRUE
 *
 * @param format formát textu
 * @param ... parametry formátování
 */
void debug_print(char *format, ...){
    // Výpis pouze pokud je DEBUG = TRUE
    if(DEBUG == TRUE){
        // Deklarace argumentů
        va_list args;
        // Naplnění argumentů
        va_start(args, format);
        // Využití argumentů
        vprintf(format, args);
        // Ukončení argumentů
        va_end(args);
    }
}