/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Ralf Reutemann
 *
 *===================================================================*/

#ifndef _HTTP_H
#define _HTTP_H

typedef enum http_method {
    HTTP_METHOD_GET = 0,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_TEST,
    HTTP_METHOD_ECHO,
    HTTP_METHOD_NOT_IMPLEMENTED,
    HTTP_METHOD_UNKNOWN
} http_method_t;


typedef enum http_status {
    HTTP_STATUS_OK = 0,                    // 200
    HTTP_STATUS_PARTIAL_CONTENT,           // 206
    HTTP_STATUS_MOVED_PERMANENTLY,         // 301
    HTTP_STATUS_NOT_MODIFIED,              // 304
    HTTP_STATUS_BAD_REQUEST,               // 400
    HTTP_STATUS_FORBIDDEN,                 // 401
    HTTP_STATUS_NOT_FOUND,                 // 404
    HTTP_STATUS_RANGE_NOT_SATISFIABLE,     // 416
    HTTP_STATUS_INTERNAL_SERVER_ERROR,     // 500
    HTTP_STATUS_NOT_IMPLEMENTED            // 501
} http_status_t;


typedef struct http_method_entry {
    char          *name;
    http_method_t  method;
} http_method_entry_t;


typedef struct http_status_entry {
    unsigned short   code;
    char            *text;
} http_status_entry_t;


extern http_method_entry_t http_method_list[];
extern http_status_entry_t http_status_list[];

#endif

