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
#include <stdlib.h>

#include "http_parser.h"

/**
 * parse the http header
 * @param 	the header as char array
 * @return 	the parsed header
 */
parsed_http_header_t
parse_http_header(char *header)
{
	parsed_http_header_t parsed_header;
	
	char delimiter[] = " ";
	char *pointer;

	//TODO: fix the parsing function

	// request method
	pointer = strtok(header, delimiter);
        printf("Length of Pointer is: %d\n", strlen(pointer));
        parsed_header.method = malloc(strlen(pointer)+1);
        strcpy(parsed_header.method, pointer);
	//memcpy(parsed_header.method, pointer, sizeof(parsed_header.method) + 1);
	// requested file
	pointer = strtok(NULL, delimiter);
        printf("Length of Pointer is: %d\n", strlen(pointer));
        parsed_header.filename = malloc(strlen(pointer)+1);
        strcpy(parsed_header.filename, pointer);
	//memcpy(parsed_header.filename, pointer, sizeof(parsed_header.filename) + 1);
	// requested protocol
	pointer = strtok(NULL, delimiter);
        printf("Length of Pointer is: %d\n", strlen(pointer));
        parsed_header.protocol = malloc(strlen(pointer)+1);
        strcpy(parsed_header.protocol, pointer);
        
        
        printf("Method: %s\nProtocol: %s\nFilename: %s\n", parsed_header.method,parsed_header.protocol,parsed_header.filename);
	//memcpy(parsed_header.protocol, pointer, sizeof(parsed_header.protocol));

	return parsed_header;
} /* end of parse_http_header */