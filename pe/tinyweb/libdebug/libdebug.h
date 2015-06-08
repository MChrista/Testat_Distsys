/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * libdebug.h - malloc/free wrapper function
 *
 * Author:  Ralf Reutemann
 * Created: 2014-05-30
 *
 *===================================================================*/


#ifndef _LIBDEBUG_H
#define _LIBDEBUG_H

/*
 * Compile-time interposition of malloc and free using C
 * preprocessor. This header file defines malloc (free)
 * as wrappers to _malloc_debug (_free_debug) respectively.
 */

#define malloc(size) _malloc_debug(size, __FILE__, __LINE__ )
#define free(ptr) _free_debug(ptr, __FILE__, __LINE__ )

void *_malloc_debug(size_t size, char *file, int line);
void _free_debug(void *ptr, char *file, int line);

#endif

