/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * wrap_malloc.c - malloc() wrapper function for static linking
 *
 * Author:  Ralf Reutemann
 * Created: 2014-05-30
 *
 *===================================================================*/


#include <stdio.h>
#include <stdlib.h>

/*
 * Link-time interposition of malloc and free using the static linker's (ld)
 * "--wrap symbol" flag.
 */

void *__real_malloc(size_t size);

/*
 * __wrap_malloc - malloc wrapper function
 */
void *
__wrap_malloc(size_t size)
{
    void *ptr = __real_malloc(size);

    fprintf(stderr, "malloc(%zd)=%p\n", size, ptr);
    return ptr;
} /* end of __wrap_malloc */

