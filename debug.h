#ifndef KIV_ZOS_DEBUG_H
#define KIV_ZOS_DEBUG_H

/*
 * Hlavičky
 */
#include <stdio.h>
#include <stdarg.h>
#include "bool.h"


/*
 * Konstanty
 */
#define DEBUG TRUE


void debug_print(char *format, ...);

#endif //KIV_ZOS_DEBUG_H
