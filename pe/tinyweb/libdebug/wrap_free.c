/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * wrap_free.c - free() wrapper function for static linking
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

void __real_free(void *ptr);

/*
 * __free_malloc - free wrapper function
 */
void
__wrap_free(void *ptr)
{
    __real_free(ptr);
    fprintf(stderr, "free(%p)\n", ptr);
} /* end of myfree */

