/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Michael Christa, Florian Hink
 *
 *===================================================================*/

#include <stdio.h>
#include <string.h>

#include "http_parser.h"

/**
 * parse the http header
 * @param 	the header as char array
 * @return 	the parsed header
 */
parsed_http_header
parse_http_header(char *header)
{
	parsed_http_header parsed_header;
	
	char delimiter[] = " ";
	char *pointer;

	pointer = strtok(header, delimiter);

	memcpy(parsed_header.method, pointer, sizeof(parsed_header.method));
	pointer = strtok(NULL, delimiter);
	memcpy(parsed_header.file, pointer, sizeof(parsed_header.file));
	pointer = strtok(NULL, delimiter);
	memcpy(parsed_header.protocol, pointer, sizeof(parsed_header.protocol));

	return parsed_header;
} /* end of parse_http_header */