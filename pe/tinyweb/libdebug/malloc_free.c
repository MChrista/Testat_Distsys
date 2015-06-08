/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * free_debug.c - free wrapper function
 *
 * Author:  Ralf Reutemann
 * Created: 2014-05-30
 *
 *===================================================================*/


#include <stdio.h>
#include <malloc.h>

#ifdef free
#undef free
#endif

/*
 *  _free_debug - free wrapper function
 */
void
_free_debug(void *ptr, char *file, int line)
{
    free(ptr);
    printf("%s:%d: free(%p)\n", file, line, ptr);
} /* end of _free_debug */

