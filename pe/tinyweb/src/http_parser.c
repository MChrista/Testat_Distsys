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
#define __USE_XOPEN
//#define _GNU_SOURCE
//#define _XOPEN_SOURCE /* Pour GlibC2 */
#include <time.h>
#include <ctype.h>
#include <math.h>


#include "http_parser.h"
#include "safe_print.h"
#include "tinyweb.h"
#include "http.h"

#define MAX_MATCHES 1
#define TRUE 1;
#define FALSE 0;

/**
 * parse the http header
 * @param 	the header as char pointer
 * @return 	the parsed header defined in http_parser.h
 */
parsed_http_header_t
parse_http_header(char *header) {

    parsed_http_header_t parsed_header;

    //Initialize State with ERROR and modsince with 0
    parsed_header.httpState = HTTP_STATUS_INTERNAL_SERVER_ERROR;
    parsed_header.modsince = 0;
    parsed_header.isCGI = FALSE;
    parsed_header.byteStart = -2;
    parsed_header.byteEnd = -2;

    char *pointer; /* Helds actual processing string */

    /*
     *  Create and compile regex to validate a correct status line
     */
    regex_t exp;
    int rv = regcomp(&exp, "^\\(GET\\|HEAD\\|POST\\|PUT\\|DELETE\\|TRACE\\|CONNECT\\|OPTIONS\\|DUMMY\\)"
            "[[:blank:]]"
            "/\\([[:alnum:]]\\|/\\|-\\)\\{0,\\}\\([[:punct:]][[:alnum:]]\\{1,\\}\\)\\{0,1\\}"
            "[[:blank:]]"
            "HTTP/[[:digit:]][[:punct:]][[:digit:]]\r$", REG_NEWLINE);
    if (rv != 0) {
        err_print("ERROR: parser regex statusline compile");
        parsed_header.httpState = HTTP_STATUS_INTERNAL_SERVER_ERROR;
        return parsed_header;
    }
    regmatch_t matches[MAX_MATCHES];
    if (regexec(&exp, header, MAX_MATCHES, matches, 0) == 0) {
        if (matches[0].rm_so == 0) { /* Match begins von first char */
            regfree(&exp);
            char delimiter[] = " ";
            pointer = strtok(header, delimiter); /* pointer points to http method */
            parsed_header.method = malloc(strlen(pointer) + 1);
            if (parsed_header.method == NULL) {
                err_print("ERROR: cant allocate memory");
                parsed_header.httpState = HTTP_STATUS_INTERNAL_SERVER_ERROR;
                return parsed_header;
            }
            strcpy(parsed_header.method, pointer);
            pointer = strtok(NULL, delimiter); /* pointer points to requested file */
            if (strlen(pointer) == 1) { /* If no file is requested */
                parsed_header.filename = DEFAULT_HTML_PAGE;
            } else {
                parsed_header.filename = malloc(strlen(pointer) + 1);
                if (parsed_header.filename == NULL) {
                    err_print("ERROR: cant allocate memory");
                    parsed_header.httpState = HTTP_STATUS_INTERNAL_SERVER_ERROR;
                    return parsed_header;
                }
                strcpy(parsed_header.filename, pointer);
                regex_t cgiReg;
                int rv = regcomp(&cgiReg, "^/cgi-bin", REG_ICASE);
                if (rv != 0) {
                    err_print("ERROR: parser regex cgi compile");
                    parsed_header.httpState = HTTP_STATUS_INTERNAL_SERVER_ERROR;
                    return parsed_header;
                }
                if (regexec(&cgiReg, parsed_header.filename, MAX_MATCHES, matches, 0) == 0) {
                    parsed_header.isCGI = TRUE;
                }
            }
            pointer = strtok(NULL, "\r"); /* \r is char before end */
            parsed_header.protocol = malloc(strlen(pointer) + 1);
            if (parsed_header.protocol == NULL) {
                    err_print("ERROR: cant allocate memory");
                    parsed_header.httpState = HTTP_STATUS_INTERNAL_SERVER_ERROR;
                    return parsed_header;
                }
            strcpy(parsed_header.protocol, pointer);

            if (strcmp(parsed_header.protocol, "HTTP/1.1") != 0) { //the only allowed Header
                parsed_header.httpState = HTTP_STATUS_BAD_REQUEST;
                return parsed_header;
            } else if (!((strcmp(parsed_header.method, "GET") == 0) || (strcmp(parsed_header.method, "HEAD") == 0))) {
                parsed_header.httpState = HTTP_STATUS_NOT_IMPLEMENTED;
                return parsed_header;
            } else {
                //Status line is correct
                parsed_header.httpState = HTTP_STATUS_OK;
            }
            //End of parsing status line

            //Start of parsing further header lines
            /*
             * Create and compile regex to parse range line
             */
            regex_t rangeRegex;
            int rv = regcomp(&rangeRegex, "^Range"
                    "[[:blank:]]\\{0,\\}"
                    ":[[:blank:]]\\{0,\\}"
                    "bytes="
                    "\\("
                    "\\([[:digit:]]\\{1,\\}-[[:digit:]]\\{0,\\}\\)"
                    "\\|"
                    "\\(-[[:digit:]]\\{1,\\}\\)"
                    "\\)", REG_ICASE);

            if (rv != 0) {
                err_print("ERROR: parser regex range compile");
                parsed_header.httpState = HTTP_STATUS_INTERNAL_SERVER_ERROR;
                return parsed_header;
            }

            /*
             * Check for further Header lines
             * If-Modified-Since and Range is implemented
             */
            pointer = strtok(NULL, "\n");
            while (pointer != NULL) {
                struct tm tm;
                char *ret = strptime(pointer, "If-Modified-Since: %a, %d %b %Y %H:%M:%S", &tm);
                if (ret != NULL) {
                    time_t t = mktime(&tm);
                    parsed_header.modsince = t;
                }

                if (regexec(&rangeRegex, pointer, MAX_MATCHES, matches, 0) == 0) {
                    int matchEnd = matches[0].rm_eo; /* Get Index of last matching char */
                    int i = 0;
                    int pos = 1; /* Parameter to set chars to right position */
                    int startValue = -1; /* -1 is not set */
                    int endValue = -1; /* -1 is not set */

                    int c = (int) (pointer[matchEnd - i - 1] - '0'); /* get last char */

                    if (c >= 0 && c <= 9) { /* If second value is set, change value to zero */
                        endValue = 0;
                    }
                    //last char is either a "-" or a digit
                    while (c >= 0 && c <= 9) { /* while Digit */
                        int temp = c;
                        endValue = endValue + temp * pos;
                        i++;
                        pos = pos * 10;
                        c = (int) (pointer[matchEnd - i - 1] - '0');
                    }
                    //Increase Index, because of -
                    i++;
                    pos = 1; /* Reset positon parameter for first value */
                    c = (int) (pointer[matchEnd - i - 1] - '0');
                    if (c >= 0 && c <= 9) {
                        startValue = 0;
                    }
                    while (c >= 0 && c <= 9) { /* Get first value */
                        int temp = c;
                        startValue = startValue + temp * pos;
                        i++;
                        pos = pos * 10;
                        c = (int) (pointer[matchEnd - i - 1] - '0');
                    }

                    /* Write data for return*/
                    parsed_header.httpState = HTTP_STATUS_RANGE_NOT_SATISFIABLE;
                    parsed_header.byteStart = startValue;
                    parsed_header.byteEnd = endValue;

                    //Check Status
                    if (startValue == -1 && endValue > 0) {
                        parsed_header.httpState = HTTP_STATUS_PARTIAL_CONTENT;
                    } else if (startValue >= 0 && endValue == -1) {
                        parsed_header.httpState = HTTP_STATUS_PARTIAL_CONTENT;
                    } else if (startValue >= 0 && endValue > 0 && startValue < endValue) {
                        parsed_header.httpState = HTTP_STATUS_PARTIAL_CONTENT;
                    }
                } //end of parsing range
                pointer = strtok(NULL, "\n");
            }

        } else {
            parsed_header.httpState = HTTP_STATUS_BAD_REQUEST;
        }
    } else {
        parsed_header.httpState = HTTP_STATUS_BAD_REQUEST;
    }
    return parsed_header;
} /* end of parse_http_header */
