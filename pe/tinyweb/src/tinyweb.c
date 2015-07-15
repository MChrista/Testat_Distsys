#define __USE_XOPEN
#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <netdb.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <getopt.h>
#include <fcntl.h>

#include "tinyweb.h"
#include "connect_tcp.h"

#include "safe_print.h"
#include "sem_print.h"

#include "http_parser.h"
#include "content.h"
#include "http.h"
#include "socket_io.h"


// Must be true for the server accepting clients,
// otherwise, the server will terminate
static volatile sig_atomic_t server_running = false;

#define IS_ROOT_DIR(mode)   (S_ISDIR(mode) && ((S_IROTH || S_IXOTH) & (mode)))

static void
print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s options\n%s%s%s%s", progname,
            "\t-d\tthe directory of web files\n",
            "\t-f\tthe logfile (if '-' or option not set; logging will be redirected to stdout\n",
            "\t-p\tthe port logging is redirected to stdout.for the server\n",
            "TIT12 Gruppe 7: Michael Christa, Florian Hink\n");
} /* end of print_usage */

static int
get_options(int argc, char *argv[], prog_options_t *opt) {
    int c;
    int err;
    int success = 1;
    char *p;
    struct addrinfo hints;

    p = strrchr(argv[0], '/');
    if (p) {
        p++;
    } else {
        p = argv[0];
    } /* end if */

    opt->progname = (char *) malloc(strlen(p) + 1);
    if (opt->progname != NULL) {
        strcpy(opt->progname, p);
    } else {
        err_print("cannot allocate memory");
        return EXIT_FAILURE;
    } /* end if */

    opt->log_filename = NULL;
    opt->root_dir = NULL;
    opt->server_addr = NULL;
    opt->verbose = 0;
    opt->timeout = 120;

    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC; /* Allows IPv4 or IPv6 */
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    while (success) {
        int option_index = 0;
        static struct option long_options[] = {
            { "file", required_argument, 0, 0},
            { "port", required_argument, 0, 0},
            { "dir", required_argument, 0, 0},
            { "verbose", no_argument, 0, 0},
            { "debug", no_argument, 0, 0},
            { NULL, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "f:p:d:hv", long_options, &option_index);
        if (c == -1) break;

        switch (c) {
            case 'f':
                // 'optarg' contains file name
                opt->log_filename = (char *) malloc(strlen(optarg) + 1);
                if (opt->log_filename != NULL) {
                    strcpy(opt->log_filename, optarg);
                } else {
                    err_print("cannot allocate memory");
                    return EXIT_FAILURE;
                } /* end if */
                break;
            case 'p':
                // 'optarg' contains port number
                if ((err = getaddrinfo(NULL, optarg, &hints, &opt->server_addr)) != 0) {
                    fprintf(stderr, "Cannot resolve service '%s': %s\n", optarg, gai_strerror(err));
                    return EXIT_FAILURE;
                } /* end if */
                opt->server_port = (int) ntohs(((struct sockaddr_in*) opt->server_addr->ai_addr)->sin_port);
                break;
            case 'd':
                // 'optarg contains root directory */
                opt->root_dir = (char *) malloc(strlen(optarg) + 1);
                if (opt->root_dir != NULL) {
                    strcpy(opt->root_dir, optarg);
                } else {
                    err_print("cannot allocate memory");
                    return EXIT_FAILURE;
                } /* end if */
                break;
            case 'h':
                break;
            case 'v':
                opt->verbose = 1;
                break;
            default:
                success = 0;
        } /* end switch */
    } /* end while */

    // check presence of required program parameters
    success = success && opt->server_addr && opt->root_dir;

    // additional parameters are silently ignored, otherwise check for
    // ((optind < argc) && success)

    return success;
} /* end of get_options */

static void
open_logfile(prog_options_t *opt) {
    // open logfile or redirect to stdout
    if (opt->log_filename != NULL && strcmp(opt->log_filename, "-") != 0) {
        opt->log_fd = fopen(opt->log_filename, "w");
        if (opt->log_fd == NULL) {
            err_print("ERROR: Cannot open logfile");
            exit(EXIT_FAILURE);
        } /* end if */
    } else {
        printf("Note: logging is redirected to stdout.\n");
        opt->log_fd = stdout;
    } /* end if */
} /* end of open_logfile */

static void
check_root_dir(prog_options_t *opt) {
    struct stat stat_buf;

    // check whether root directory is accessible
    if (stat(opt->root_dir, &stat_buf) < 0) {
        /* root dir cannot be found */
        err_print("ERROR: Cannot access root dir");
        exit(EXIT_FAILURE);
    } else if (!IS_ROOT_DIR(stat_buf.st_mode)) {
        err_print("Root dir is not readable or not a directory");
        exit(EXIT_FAILURE);
    } /* end if */
} /* end of check_root_dir */

static void
sig_handler(int sig) {
    int status;
    switch (sig) {
        case SIGINT:
            // use our own thread-safe implemention of printf
            safe_printf("\n[%d] Server terminated due to keyboard interrupt\n", getpid());
            server_running = false;
            break;
        case SIGCHLD:
            while (waitpid(-1, &status, WNOHANG) > 0) {
            }
            break;
        case SIGSEGV:
            safe_printf("\n[%d] Server terminated due to segmentation fault\n", getpid());
            server_running = false;
            break;
        case SIGABRT:
            safe_printf("\n[%d] Server terminated due to system abort\n", getpid());
            server_running = false;
            break;
        default:
            break;
    } /* end switch */
} /* end of sig_handler */

static void
install_signal_handlers(void) {
    struct sigaction sa;

    // init signal handler(s)
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sig_handler;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        err_print("sigaction(SIGINT)");
        exit(EXIT_FAILURE);
    } /* end if */
} /* end of install_signal_handlers */

/**
 * Creates a server socket.
 * @param   the program options
 * @return  the socket descriptor
 */
int
create_server_socket(prog_options_t *server) {
    int sfd; /* socket file descriptor */
    int retcode; /* return code from bind */
    const int on = 1; /* used to set socket option */
    const int qlen = 5; /* socket descriptor */

    /*
     * Create a socket
     */
    sfd = socket(PF_INET, SOCK_STREAM, server->server_addr->ai_protocol);
    if (sfd < 0) {
        err_print("ERROR: server socket()");
        return -1;
    } /* end if */

    /*
     * Set socket options.
     */
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));

    /*
     * Bind the socket to the provided port.
     */
    retcode = bind(sfd, server->server_addr->ai_addr, server->server_addr->ai_addrlen);
    if (retcode < 0) {
        err_print("ERROR: server bind()");
        return -1;
    } /* end if */

    /*
     * Place the socket in passive mode.
     */
    retcode = listen(sfd, qlen);
    if (retcode < 0) {
        err_print("ERROR: server listen()");
        return -1;
    } /* end if */

    return sfd;
} /* end of create_server_socket */

/**
 * write response header to the client
 * @input_param     the socket descriptor
 * @input_param     the response header string
 * @input_param     the program options
 * @return          unequal zero in case of error
 */
static int
write_response_header(int sd, char *response_header_string, prog_options_t *server) {
    int retcode;

    /*
     * write header
     */
    retcode = write_to_socket(sd, response_header_string, strlen(response_header_string), server->timeout);
    if (retcode < 0) {
        err_print("ERROR: write()");
        return -1;
    } /* end if */


    // TODO: return something different than 0
    return 0;
} /* end of write_response_header */

/*
 * write the response body to client
 * @input_param     the socket descriptor
 * @input_param     the path to the requested file
 * @input_param     the program options
 * @input_param     the start of partial content
 * @input_param     the end of partial content
 * @return          unequal zero in case of error
 */
static int
write_response_body(int sd, char *filepath, prog_options_t *server, int start, int end) {
    int retcode;
    int file; /* file descriptor of requested file */
    int chunkSize = 256; /* chunk size for write of message body */

    file = open(filepath, O_RDONLY);
    if (file < 0) {
        err_print("ERROR: open()");
        return -1;
    } /* end if */

    char chunk[chunkSize];
    struct stat fstat; /* file status */
    retcode = stat(filepath, &fstat);
    size_t bytesWritten = 0;
    size_t bytesToWrite = fstat.st_size;

    if ((start != -2) && (end != -2)) { /* for partial content */
        bytesWritten = start;
        if (end == -1) {
            bytesToWrite = fstat.st_size;
        } else {
            bytesToWrite = end;
        }

        if (lseek(file, bytesWritten, SEEK_SET) < 0) {
            return -1;
        }
    }

    while (bytesWritten < bytesToWrite) {
        size_t writtenThisTime;
        size_t readThisTime;

        readThisTime = read(file, chunk, chunkSize);

        if (readThisTime < 0) {
            return -1;
        }

        writtenThisTime = write(sd, chunk, readThisTime);
        if (writtenThisTime == -1) {
            return -1;
        }

        bytesWritten += writtenThisTime;
    }

    return retcode;
} /* end of write_response_body */

static int
create_response_header(char *filepath, http_header_t *response_header_data, struct stat fstat, int start, int end) {

    // content-type
    char *content_type_str;
    http_content_type_t content_type;
    content_type = get_http_content_type(filepath);
    content_type_str = get_http_content_type_str(content_type);
    int size = strlen(http_header_field_list[4]) + strlen(content_type_str) + strlen("\r\n") + 1;
    response_header_data->content_type = malloc(size);
    snprintf(response_header_data->content_type, size, "%s%s\r\n", http_header_field_list[4], content_type_str);


    struct tm * timeinfo;
    char timeString[80];
    timeinfo = localtime(&fstat.st_mtime);
    strftime(timeString, 80, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
    size = strlen(http_header_field_list[2]) + strlen(timeString) + strlen("\r\n") + 1;
    response_header_data->last_modified = malloc(size);
    snprintf(response_header_data->last_modified, size, "%s%s\r\n", http_header_field_list[2], timeString);

    // content-range
    if ((start != -2) && (end != -2)) { /* for partial content */
        size = strlen(http_header_field_list[8]) + sizeof (int)*3 + strlen("bytes -/\r\n") + 1;
        response_header_data->content_range = malloc(size);
        int endOfRange = fstat.st_size - 1;
        snprintf(response_header_data->content_range, size, "%sbytes %d-%d/%d\r\n", http_header_field_list[8], start, endOfRange, (int) fstat.st_size);

        int size = strlen(http_header_field_list[3]) + sizeof (int) +strlen("\r\n") + 1;
        int range_length = fstat.st_size - start;
        response_header_data->content_length = malloc(size);
        snprintf(response_header_data->content_length, size, "%s%d\r\n", http_header_field_list[3], range_length);
    } else {
        // content-length, content type, und last modified
        size = strlen(http_header_field_list[3]) + sizeof (long long) +strlen("\r\n") + 1;
        response_header_data->content_length = malloc(size);
        snprintf(response_header_data->content_length, size, "%s%lld\r\n", http_header_field_list[3], (long long) fstat.st_size);
    }

    // TODO: return retcode instead of 0
    return 0;
} /* end of create_response_header */

/**
 * create the response header string
 * @input_param     the response header data
 * @output_param    the response header string
 * @return          unequal zero in case of error
 */
static int
create_response_header_string(http_header_t response_header_data, char* response_header_string) {
    // status
    snprintf(response_header_string, 50, "%s %hu %s\r\n", "HTTP/1.1", response_header_data.status.code, response_header_data.status.text);

    // server
    char server[30];
    snprintf(server, 30, "%s%s\r\n", http_header_field_list[1], "Tinyweb 1.1");
    strcat(response_header_string, server);

    // date
    char timeString [80];
    char date [50];
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeString, 80, "%a, %d %b %Y %H:%M:%S", timeinfo);
    snprintf(date, 50, "%s%s\r\n", http_header_field_list[0], timeString);
    strcat(response_header_string, date);

    if (response_header_data.content_length != NULL) {
        //content length
        strcat(response_header_string, response_header_data.content_length);
    }
    if (response_header_data.content_type != NULL) {
        //content type
        strcat(response_header_string, response_header_data.content_type);
    }
    if (response_header_data.last_modified != NULL) {
        //last modified
        strcat(response_header_string, response_header_data.last_modified);
    }
    if (response_header_data.content_location != NULL) {
        strcat(response_header_string, response_header_data.content_location);
    }
    if (response_header_data.content_range != NULL) {
        strcat(response_header_string, response_header_data.content_range);
    }
    // end header
    strcat(response_header_string, "\r\n");
    return 0;
} /* end of create_response_header_string */

/**
 * write log to stdout or log file
 * @input_param     the parsed http header
 * @input_param     the client info
 * @input_param     the path to requested file
 * @input_param     the response header string
 * @input_param     the file status
 * @input_param     the start of partial content
 * @input_param     the end of partial content
 * @input_param     the program options
 * @return          unequal zero in case of error
 */
static int
write_log(http_status_entry_t httpStatus, parsed_http_header_t parsed_header, struct sockaddr_in client, char* filepath, char* response_header_string, struct stat fstat, int start, int end, prog_options_t *server) {
    /*
     * write log
     */
    // time
    char timeString [80];
    char date [50];
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeString, 80, "%a, %d %b %Y %H:%M:%S", timeinfo);
    snprintf(date, 50, "%s +0200", timeString);
    // IP Address
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client.sin_addr), str, INET_ADDRSTRLEN);
    // Port
    int portNumber = ntohs(client.sin_port);
    int size = strlen(response_header_string);
    if (server->log_filename != NULL && strcmp(server->log_filename, "-") != 0) { /* write to logfile*/
        if ((start != -2) && (end != -2)) { /* for partial requests only */
            size += fstat.st_size - start;
            print_log("[%d] %s:%d - - [%s] \"%-7s %s %s\" %d %d\n", getpid(), str, portNumber, date, parsed_header.method, filepath, parsed_header.protocol, httpStatus.code , size);
        } else { /* every other request */
            size += fstat.st_size;
            print_log("[%d] %s:%d - - [%s] \"%-7s %s %s\" %d %d\n", getpid(), str, portNumber, date, parsed_header.method, filepath, parsed_header.protocol, httpStatus.code , size);
        }
    } else { /* write to stdout*/
        if ((start != -2) && (end != -2)) { /* for partial requests only */
            size += fstat.st_size - start;
            safe_printf("[%d] %s:%d - - [%s] \"%-7s %s %s\" %d %d\n", getpid(), str, portNumber, date, parsed_header.method, filepath, parsed_header.protocol, httpStatus.code , size);
        } else { /* every other request */
            size += fstat.st_size;
            safe_printf("[%d] %s:%d - - [%s] \"%-7s %s %s\" %d %d\n", getpid(), str, portNumber, date, parsed_header.method, filepath, parsed_header.protocol, httpStatus.code , size);
        }
    }
    return 0;
} /*end of write_log */

/**
 * Handle clients.
 * @input_param     the socket descriptor to read on
 * @input_param     the program options
 * @return          on error -1 is returned
 */
static int
handle_client(int sd, prog_options_t *server, struct sockaddr_in client) {
    int BUFSIZE = 1000;
    char client_header[BUFSIZE];
    char server_header[BUFSIZE];
    parsed_http_header_t parsed_header;
    http_header_t response_header_data = {
        .status = http_status_list[8],
        .date = NULL,
        .server = NULL,
        .last_modified = NULL,
        .content_length = NULL,
        .content_type = NULL,
        .connection = NULL,
        .accept_ranges = NULL,
        .content_location = NULL,
        .content_range = NULL
    };
    int retcode = 0;
    char filepath[BUFSIZE]; /* path to requested file */
    struct stat fstat; /* file status */

    read_from_socket(sd, client_header, BUFSIZE, server->timeout);
    parsed_header = parse_http_header(client_header);

    // check on parsed http status
    switch (parsed_header.httpState) {
        case HTTP_STATUS_INTERNAL_SERVER_ERROR:
            response_header_data.status = http_status_list[8];
            create_response_header_string(response_header_data, server_header);
            write_log(http_status_list[8], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
            return write_response_header(sd, server_header, server);
        case HTTP_STATUS_BAD_REQUEST:
            response_header_data.status = http_status_list[4];
            create_response_header_string(response_header_data, server_header);
            write_log(http_status_list[4], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
            return write_response_header(sd, server_header, server);
        case HTTP_STATUS_NOT_IMPLEMENTED:
            response_header_data.status = http_status_list[9];
            create_response_header_string(response_header_data, server_header);
            write_log(http_status_list[9], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
            return write_response_header(sd, server_header, server);
        default:
            break;
    }

    strcpy(filepath, server->root_dir);
    strcat(filepath, parsed_header.filename);
    retcode = stat(filepath, &fstat);

    if (retcode) {
        response_header_data.status = http_status_list[6];
        create_response_header_string(response_header_data, server_header);
        write_log(http_status_list[6], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
        return write_response_header(sd, server_header, server);
    }

    switch (parsed_header.httpState) {
        case HTTP_STATUS_RANGE_NOT_SATISFIABLE:
            response_header_data.status = http_status_list[7];
            create_response_header_string(response_header_data, server_header);
            write_log(http_status_list[7], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
            return write_response_header(sd, server_header, server);
        case HTTP_STATUS_PARTIAL_CONTENT:
            if (parsed_header.byteStart >= fstat.st_size) { /* throw 416 */
                response_header_data.status = http_status_list[7];
                create_response_header_string(response_header_data, server_header);
                write_log(http_status_list[7], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
                return write_response_header(sd, server_header, server);
            }
            response_header_data.status = http_status_list[1];
            create_response_header(filepath, &response_header_data, fstat, parsed_header.byteStart, parsed_header.byteEnd);
            create_response_header_string(response_header_data, server_header);
            write_response_header(sd, server_header, server);
            write_log(http_status_list[1], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
            return write_response_body(sd, filepath, server, parsed_header.byteStart, parsed_header.byteEnd);
        default:
            break;
    }

    // check for 404, 304, 301
    if (!(S_ISREG(fstat.st_mode)) && !(S_ISDIR(fstat.st_mode))) { /* 404 */
        response_header_data.status = http_status_list[6];
        create_response_header_string(response_header_data, server_header);
        write_log(http_status_list[6], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
        return write_response_header(sd, server_header, server);
    } else if (S_ISDIR(fstat.st_mode)) { /* 301 */
        response_header_data.status = http_status_list[2];
        int size = strlen(http_header_field_list[7]) + strlen(filepath) + strlen("/\r\n") + 1;
        response_header_data.content_location = malloc(size);
        snprintf(response_header_data.content_location, BUFSIZE, "%s%s%s\r\n", http_header_field_list[7], filepath, "/");
        create_response_header_string(response_header_data, server_header);
        write_log(http_status_list[2], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
        return write_response_header(sd, server_header, server);
    } else if (parsed_header.modsince != 0) { /* 304 */
        int seconds;
        seconds = difftime(parsed_header.modsince, fstat.st_mtime);
        if (seconds >= 0) {
            response_header_data.status = http_status_list[3];
            create_response_header_string(response_header_data, server_header);
            write_log(http_status_list[3], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
            return write_response_header(sd, server_header, server);
        }
    }

    // already checked for 404
    if (parsed_header.isCGI) {
        pid_t pid; /* process id */
        //int link[2];
        //char buffer[4096];
        /*
         * Check executable, only on success go on
         */
        if (fstat.st_mode & S_IEXEC) {
            char* execPath = malloc(strlen(filepath) + 3);
            if (execPath == NULL) {
                err_print("ERROR: cant allocate memory");
            }
            strcpy(execPath, "./");
            strcat(execPath, filepath);

            pid = fork();
            if (pid == 0) {
                /* 
                 * child process 
                 */
                dup2(sd, STDOUT_FILENO);
                close(sd);

                response_header_data.status = http_status_list[0];
                create_response_header_string(response_header_data, server_header);

                int headerLength = strlen(server_header);
                server_header[headerLength - 2] = '\0'; /* cut off one \r\n */

                fprintf(stdout, "%s", server_header); /* print header */
                execle("/bin/sh", "sh", "-c", execPath, NULL, NULL);
                exit(EXIT_SUCCESS);

                //exit(EXIT_SUCCESS);
            } else if (pid > 0) {

                /* 
                 * parent process 
                 */

                int status;
                wait(&status);
                if (WIFEXITED(status) != 1) {
                    err_print("ERROR: Execute cgi failed");
                    return -1;
                }
                close(sd);
                exit(EXIT_SUCCESS);
            } else {
                /* 
                 * error while forking 
                 */
                err_print("ERROR: fork() in cgi");
                return 1;
            }
        } else { /* 403 - Not executable*/
            response_header_data.status = http_status_list[5];
            create_response_header_string(response_header_data, server_header);
            write_log(http_status_list[5], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
            return write_response_header(sd, server_header, server);
        }
    }

    // check on parsed http method
    if (strcmp(parsed_header.method, "GET") == 0) { /* GET method */
        response_header_data.status = http_status_list[0];
        create_response_header(filepath, &response_header_data, fstat, parsed_header.byteStart, parsed_header.byteEnd);
        create_response_header_string(response_header_data, server_header);
        write_response_header(sd, server_header, server);
        write_log(http_status_list[0], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
        return write_response_body(sd, filepath, server, parsed_header.byteStart, parsed_header.byteEnd);
    } else { /* HEAD method */
        response_header_data.status = http_status_list[0];
        create_response_header(filepath, &response_header_data, fstat, parsed_header.byteStart, parsed_header.byteEnd);
        create_response_header_string(response_header_data, server_header);
        write_log(http_status_list[0], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
        return write_response_header(sd, server_header, server);
    }

    response_header_data.status = http_status_list[8];
    create_response_header_string(response_header_data, server_header);
    write_log(http_status_list[8], parsed_header, client, filepath, server_header, fstat, parsed_header.byteStart, parsed_header.byteEnd, server);
    return write_response_header(sd, server_header, server);

} /* end of handle_client */

/**
 * Accept clients on the socket.
 * @input_param     the socket descriptor
 * @input_param     the program options
 * @return          unequal zero in case of error
 */
static int
accept_client(int sd, prog_options_t *server) {

    signal(SIGCHLD, sig_handler);

    int nsd; /* new socket descriptor */
    pid_t pid; /* process id */
    int retcode; /* return code */
    struct sockaddr_in client; /* the input sockaddr */
    socklen_t client_len = sizeof (client); /* the length of it */

    /*
     * accept clients on the socket
     */
    nsd = accept(sd, (struct sockaddr *) &client, &client_len);
    if (nsd < 0) {
        err_print("ERROR: server accept()");
        return -1;
    }

    pid = fork();
    if (pid == 0) {
        /* 
         * child process 
         */
        retcode = close(sd);
        if (retcode < 0) {
            err_print("ERROR: child close()");
        } /* end if */
        retcode = handle_client(nsd, server, client);
        if (retcode < 0) {
            err_print("ERROR: child handle_client()");
            exit(EXIT_FAILURE);
        } /* end if */
        exit(EXIT_SUCCESS);
    } else if (pid > 0) {
        /* 
         * parent process 
         */
        retcode = close(nsd);
        if (retcode < 0) {
            err_print("ERROR: parent close()");
        } /* end if */
    } else {
        /* 
         * error while forking 
         */
        err_print("ERROR: fork()");
    }

    return nsd;
} /* end of accept_client */

/**
 * Main function
 * @input_param     the argument counter
 * @input_param     the arugment vector
 * @return          the Exit Code  
 */
int
main(int argc, char *argv[]) {
    int retcode = EXIT_SUCCESS;
    prog_options_t my_opt;
    int socketDescriptor;

    // read program options
    if (get_options(argc, argv, &my_opt) == 0) {
        print_usage(my_opt.progname);
        exit(EXIT_FAILURE);
    } /* end if */

    // set the time zone (TZ) to GMT in order to
    // ignore any other local time zone that would
    // interfere with correct time string parsing
    setenv("TZ", "GMT", 1);
    tzset();

    // do some checks and initialisations...
    open_logfile(&my_opt);
    check_root_dir(&my_opt);
    install_signal_handlers();
    init_logging_semaphore(&my_opt);

    // create the server socket
    socketDescriptor = create_server_socket(&my_opt);
    if (retcode < 0) {
        err_print("ERROR: creating socket()");
        exit(retcode);
    } /* end if */

    // here, as an example, show how to interact with the
    // condition set by the signal handler above
    safe_printf("[%d] Starting server '%s'...\n", getpid(), my_opt.progname);
    server_running = true;
    while (server_running) {
        // TODO: add error handling to accept_client
        retcode = accept_client(socketDescriptor, &my_opt);
        if (retcode < 0) {
            err_print("ERROR: accepting clients()");
            exit(retcode);
        } /* end if */
    } /* end while */

    safe_printf("[%d] Good Bye...", getpid());
    return retcode;
} /* end of main */