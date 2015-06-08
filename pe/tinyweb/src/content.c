/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Ralf Reutemann
 *
 *===================================================================*/

#include <string.h>
#include "content.h"


static http_content_type_entry_t http_content_type_list[] = {
    { ".html",    "text/html"          },
    { ".css",     "text/css"           },
    { ".gif",     "image/gif"          },
    { ".jpg",     "image/jpeg"         },
    { ".pdf",     "application/pdf"    },
    { ".tar",     "application/x-tar"  },
    { ".xml",     "application/xml"    },
    { NULL,       "text/plain"         }
};


http_content_type_t
get_http_content_type(const char *filename)
{
    int i;

    i = 0;
    while (http_content_type_list[i].ext != NULL) {
        if (strstr(filename, http_content_type_list[i].ext)) {
            break;
        } /* end if */
        i++;
    } /* end while */

    return (http_content_type_t)i;
} /* end of get_http_content_type */


char *
get_http_content_type_str(const http_content_type_t type)
{
    return http_content_type_list[type].name;
} /* end of get_http_content_type_str */

