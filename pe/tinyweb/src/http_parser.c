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
#include <sys/types.h>
#include <regex.h>

#include "http_parser.h"

#define MAX_MATCHES 1

/**
 * parse the http header
 * @param 	the header as char array
 * @return 	the parsed header
 */
parsed_http_header_t
parse_http_header(char *header) {
    parsed_http_header_t parsed_header;

    char delimiter[] = " ";
    char *pointer;

    //TODO: fix the parsing function
    regex_t exp;
    int rv = regcomp(&exp, "^\\(GET\\|HEAD\\)"
    "[[:blank:]]"
    "/\\([[:alnum:]]\\|/\\)\\{1,\\}\\([[:punct:]][[:alnum:]]\\{1,\\}\\)\\{0,1\\}"
    "[[:blank:]]"
    "HTTP/[[:digit:]][[:punct:]][[:digit:]]\r$", REG_NEWLINE);
    if (rv != 0) {
        printf("regcomp failed with %d\n", rv);
    }
    regmatch_t matches[MAX_MATCHES];
    char *sz = header;
    if (regexec(&exp, sz, MAX_MATCHES, matches, 0) == 0) {
        if(matches[0].rm_so == 0){
            regfree(&exp);
            pointer = strtok(header, delimiter);
            parsed_header.method = malloc(strlen(pointer) + 1);
            strcpy(parsed_header.method, pointer);
            pointer = strtok(NULL, delimiter);
            parsed_header.filename = malloc(strlen(pointer) + 1);
            strcpy(parsed_header.filename, pointer);
            pointer = strtok(NULL, "\r");
            parsed_header.protocol = malloc(strlen(pointer) + 1);
            strcpy(parsed_header.protocol, pointer);
            /*
             * TODO: Handle other Header Fields
             * DATE
             * RANGE
             * 
             */
            pointer = strtok(NULL, "\n");
            while(ptr != NULL) {
                parseHeaderField(pointer);
                ptr = strtok(NULL, delimiter);
            }   
            
        }else{
            printf("Status line was in false line\n");
            parsed_header.method = "BAD REQUEST";
        }
    } else {
        printf("\"%s\" does not match\n", sz);
        parsed_header.method = "BAD REQUEST";
    }
    //printf("Parsed Header Parameters are:\nProtkoll: %s\nFilename: %s\nMethod: %s\n", parsed_header.protocol,parsed_header.filename,parsed_header.method);
    return parsed_header;
} /* end of parse_http_header */

void parseHeaderField(char *str){
    regex_t exp;
    int rv = regcomp(&exp, "^[[:alnum:]]\\{1,\\}", REG_NEWLINE);
    if (rv != 0) {
        printf("regcomp failed with %d\n", rv);
    }
    regmatch_t matches[MAX_MATCHES];
    if (regexec(&exp, sz, MAX_MATCHES, matches, 0) == 0) {
        
        
        
    }   
}