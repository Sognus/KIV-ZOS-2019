#ifndef KIV_ZOS_DEBUG_H
#define KIV_ZOS_DEBUG_H

/*
 * Hlavičky
 */
#include <stdio.h>
#include <stdarg.h>
#include "bool.h"

// Chování při zápisu do logovacího souboru
#define LOG_MODE_APPEND "a"
#define LOG_MODE_REWRITE "w+"

// Nastavení logovacího souboru
#define LOG_FILE "vfs.log"
#define LOG_FILE_ENABLED TRUE
#define LOG_FILE_TIME_ENABLED TRUE
#define LOG_MODE_CURRENT LOG_MODE_APPEND

// Nastavení výpisu do terminálu
#define STDOUT_ENABLED TRUE
#define STDOUT_TIME_ENABLED FALSE

// Definice levelů výpisu
#define LOG_ALL 0
#define LOG_TRACE 1
#define LOG_DEBUG 2
#define LOG_INFO 3
#define LOG_ERROR 4
#define LOG_FATAL 5

/*
 * Konstanty
 */
#define DEBUG TRUE
#define DEBUG_LEVEL LOG_ALL

// Definice formátování
#define LOG_TIME_FORMAT "%d.%m.%Y %H:%M:%S"

/**
 * Wrapper pro funkci printf, výpis pouze při DEBUG=TRUE
 * a DEBUG_LEVEL >= 1
 *
 * @param format formát textu
 * @param ... parametry formátování
 */
void log_trace(char *format, ...);

/**
 * Wrapper pro funkci printf, výpis pouze při DEBUG=TRUE
 * a DEBUG_LEVEL >= 2
 *
 * @param format formát textu
 * @param ... parametry formátování
 */
void log_debug(char *format, ...);

/**
 * Wrapper pro funkci printf, výpis pouze při DEBUG=TRUE
 * a DEBUG_LEVEL >= 3
 *
 * @param format formát textu
 * @param ... parametry formátování
 */
void log_info(char *format, ...);

/**
 * Wrapper pro funkci printf, výpis pouze při DEBUG=TRUE
 * a DEBUG_LEVEL >= 4
 *
 * @param format formát textu
 * @param ... parametry formátování
 */
void log_error(char *format, ...);

/**
 * Wrapper pro funkci printf, výpis pouze při DEBUG=TRUE
 * a DEBUG_LEVEL >= 3
 *
 * @param format formát textu
 * @param ... parametry formátování
 */
void log_fatal(char *format, ... );

#endif //KIV_ZOS_DEBUG_H
