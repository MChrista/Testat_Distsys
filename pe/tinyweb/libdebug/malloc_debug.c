/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * malloc_debug.c - malloc/free wrapper functions
 *
 * Author:  Ralf Reutemann
 * Created: 2014-05-30
 *
 *===================================================================*/


#include <stdio.h>
#include <malloc.h>

#ifdef malloc
#undef malloc
#endif

/*
 *  _malloc_debug - malloc wrapper function
 */
void *
_malloc_debug(size_t size, char *file, int line)
{
    void *ptr = malloc(size);

    printf("%s:%d: malloc(%zd)=%p\n", file, line, size, ptr);
    return ptr;
} /* end of _malloc_debug */

