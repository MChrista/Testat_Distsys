/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Ralf Reutemann
 *
 *===================================================================*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

/*
 * safe_printf - async-signal-safe wrapper for printf
 */
#define MAXS 1024
int
safe_printf(const char *format, ...)
{
    char buf[MAXS];
    ssize_t cc;
    va_list args;

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    cc = write(STDOUT_FILENO, buf, strlen(buf)); /* write is async-signal-safe */

    return (int)cc;
} /* end of safe_printf */

