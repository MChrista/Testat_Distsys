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
#include "safe_print.h"
#include "tinyweb.h"
#include "http.h"

#define MAX_MATCHES 1

/**
 * parse the http header
 * @param 	the header as char array
 * @return 	the parsed header
 */
parsed_http_header_t
parse_http_header(char *header) {
    
    
    
    parsed_http_header_t parsed_header;
    
    //Initialize State with ERROR
    parsed_header.httpState = HTTP_STATUS_INTERNAL_SERVER_ERROR;
    
    char delimiter[] = " ";
    char *pointer;
    //TODO: fix the parsing function
    regex_t exp;
    int rv = regcomp(&exp, "^\\(GET\\|HEAD\\|POST\\|PUT\\|DELETE\\|TRACE\\|CONNECT\\)"
    "[[:blank:]]"
    "/\\([[:alnum:]]\\|/\\)\\{0,\\}\\([[:punct:]][[:alnum:]]\\{1,\\}\\)\\{0,1\\}"
    "[[:blank:]]"
    "HTTP/[[:digit:]][[:punct:]][[:digit:]]\r$", REG_NEWLINE);
    if (rv != 0) {
        printf("regcomp failed with %d\n", rv);
    }
    regmatch_t matches[MAX_MATCHES];
    if (regexec(&exp, header, MAX_MATCHES, matches, 0) == 0) {
        if(matches[0].rm_so == 0){
            regfree(&exp);
            pointer = strtok(header, delimiter);
            parsed_header.method = malloc(strlen(pointer) + 1);
            strcpy(parsed_header.method, pointer);
            pointer = strtok(NULL, delimiter);
            if(strlen(pointer) == 1){
                parsed_header.filename = DEFAULT_HTML_PAGE;
            }else{
                parsed_header.filename = malloc(strlen(pointer) + 1);
                strcpy(parsed_header.filename, pointer);
            }
            pointer = strtok(NULL, "\r");
            parsed_header.protocol = malloc(strlen(pointer) + 1);
            strcpy(parsed_header.protocol, pointer);
            
            if(strcmp(parsed_header.protocol,"HTTP/1.1") != 0){ //the only allowed Header
                parsed_header.httpState = HTTP_STATUS_BAD_REQUEST;
            }else if(!((strcmp(parsed_header.method,"GET") == 0) || (strcmp(parsed_header.method,"HEAD") == 0))) {
                parsed_header.httpState = HTTP_STATUS_NOT_IMPLEMENTED;
            }else{
                //Status line is correct
                parsed_header.httpState = HTTP_STATUS_OK;
            }
            /*
             * TODO: Handle other Header Fields
             * DATE
             * RANGE
             * 
             */
            pointer = strtok(NULL, "\n");
            while(pointer != NULL) {
                parseHeaderField(pointer);
                pointer = strtok(NULL, delimiter);
            }
            
        }else{
            safe_printf("Status line was false\n");
            parsed_header.httpState = HTTP_STATUS_BAD_REQUEST;
        }
    } else {
        safe_printf("does not match\n");
        parsed_header.httpState = HTTP_STATUS_BAD_REQUEST;
    }
    return parsed_header;
} /* end of parse_http_header */

void parseHeaderField(char *str){
    /*
     * Range - Wenn außerhalb dann 416 - bei erfolg 206
     * If-Modified-Since - sonst 304 ohne ressource
     * 
     */
    regex_t rangeRegex;
    int rv = regcomp(&rangeRegex, "^Range"
    "[[:blank:]]\\{0,\\}"
    ":[[:blank:]]\\{0,\\}"
    "bytes=[[:digit:]]\\{1,\\}-\\?", REG_ICASE);
    if (rv != 0) {
        safe_printf("regcomp failed with %d\n", rv);
    }
    char *sz="Range: bytes=1--\n";
    regmatch_t matches[MAX_MATCHES];
    if (regexec(&rangeRegex, sz, MAX_MATCHES, matches, 0) == 0) {
        //safe_printf("begin: %d end: %d",matches[0].rm_so,matches[0].rm_eo);
    }   
}