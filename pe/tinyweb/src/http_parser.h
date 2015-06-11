/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Michael Christa, Florian Hink
 *
 *===================================================================*/

#ifndef _HTTP_PARSER_H
#define _HTTP_PARSER_H

typedef struct parsed_http_header
{
	char method[5];
	char filename[50];
	char protocol[9];
} parsed_http_header_t;

extern parsed_http_header_t parse_http_header(char *header);

#endif