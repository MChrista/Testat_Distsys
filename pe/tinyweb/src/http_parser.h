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
	char* method;
	char* filename;
	char* protocol;
} parsed_http_header_t;

char* default_file = "/index.html";

extern parsed_http_header_t parse_http_header(char *header);
void parseHeaderField(char *);
#endif