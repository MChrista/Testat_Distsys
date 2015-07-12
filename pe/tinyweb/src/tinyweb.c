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

        c = getopt_long(argc, argv, "f:p:d:h:v", long_options, &option_index);
        // TODO: ausführlicher
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
                // TODO: print out help
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
print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s options\n%s%s%s%s", progname,
            "\t-d\tthe directory of web files\n",
            "\t-f\tthe logfile (if '-' or option not set; logging will be redirected to stdout\n",
            "\t-p\tthe port logging is redirected to stdout.for the server\n",
            "TIT12 Gruppe 7: Michael Christa, Florian Hink\n");
} /* end of print_usage */

static void
open_logfile(prog_options_t *opt) {
    // open logfile or redirect to stdout
    if (opt->log_filename != NULL && strcmp(opt->log_filename, "-") != 0) {
        opt->log_fd = fopen(opt->log_filename, "w");
        if (opt->log_fd == NULL) {
            perror("ERROR: Cannot open logfile");
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
        perror("ERROR: Cannot access root dir");
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
                //safe_printf("terminated child\n");
                //safe_printf("%d\n", status);
                // TODO if status != 0 return 503?
            }
            break;
            // TODO: Complete signal handler
        default:
            break;
    } /* end switch */
} /* end of sig_handler */

static void
install_signal_handlers(void) {
    struct sigaction sa;

    // init signal handler(s)
    // TODO: add other signals
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sig_handler;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction(SIGINT)");
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
        perror("ERROR: server socket()");
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
        perror("ERROR: server bind()");
        return -1;
    } /* end if */

    /*
     * Place the socket in passive mode.
     */
    retcode = listen(sfd, qlen);
    if (retcode < 0) {
        perror("ERROR: server listen()");
        return -1;
    } /* end if */

    return sfd;
} /* end of create_server_socket */

static int
write_response_header(int sd, char *response_header_string, prog_options_t *server) {
    int retcode;

    /*
     * write header
     */
    retcode = write_to_socket(sd, response_header_string, strlen(response_header_string), server->timeout);
    if (retcode < 0) {
        perror("ERROR: write()");
        return -1;
    } /* end if */

    return 0;
}

static int
write_response_body(int sd, char *filepath, prog_options_t *server) {
    int retcode;
    int file; /* file descriptor of requested file */
    int chunkSize = 256; /* chunk size for write of message body */

    safe_printf("%s\n", filepath);

    file = open(filepath, O_RDONLY);
    if (file < 0) {
        perror("ERROR: open()");
        return -1;
    } /* end if */

    char chunk[chunkSize];
    struct stat fstat; /* file status */
    retcode = stat(filepath, &fstat);
    size_t bytesWritten = 0;
    size_t bytesToWrite = fstat.st_size;

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
}

static int
create_response_header(char *filepath, http_header_t response_header_data, struct stat fstat) {
    safe_printf("create response header");
    // content-length, content type, und last modified
    int size = strlen(http_header_field_list[3]) + sizeof (long long) +strlen("\r\n") + 1;
    response_header_data.content_length = malloc(size);
    snprintf(response_header_data.content_length, size, "%s%lld\r\n", http_header_field_list[3], (long long) fstat.st_size);
    safe_printf("create response header");
    safe_printf("Content length: %s\n", response_header_data.content_length);

    // content-type
    char *content_type_str;
    http_content_type_t content_type;
    content_type = get_http_content_type(filepath);
    content_type_str = get_http_content_type_str(content_type);
    size = strlen(http_header_field_list[4]) + strlen(content_type_str) + strlen("\r\n") + 1;
    response_header_data.content_type = malloc(size);
    snprintf(response_header_data.content_type, size, "%s%s\r\n", http_header_field_list[4], content_type_str);
    //snprintf(response_header_data.last_modified)
    safe_printf("Content Type: %s\n", response_header_data.content_type);

    struct tm * timeinfo;
    char timeString[80];
    timeinfo = localtime(&fstat.st_mtime);
    strftime(timeString, 80, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
    size = strlen(http_header_field_list[2]) + strlen(timeString) + strlen("\r\n") + 1;
    response_header_data.last_modified = malloc(size);
    snprintf(response_header_data.last_modified, size, "%s%s\r\n", http_header_field_list[2], timeString);
    safe_printf("Last modified: %s\n", response_header_data.last_modified);
    return 0;
}

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
        strcat(response_header_string, response_header_data.content_length);
    }
    if (response_header_data.content_length != NULL) {
        //last modified
        strcat(response_header_string, response_header_data.content_length);
    }

    // end header
    strcat(response_header_string, "\r\n");
    safe_printf("Response Header\n%s\n\n",response_header_string);
    return 0;
}

/**
 * Handle clients.
 * @param   the socket descriptor to read on
 * @param   the program options
 * @return  on error -1 is returned
 */
static int
handle_client(int sd, prog_options_t *server, struct sockaddr_in client) {
    int BUFSIZE = 1000;
    char client_header[BUFSIZE];
    char server_header[BUFSIZE];
    parsed_http_header_t parsed_header;
    http_header_t response_header_data = {
        .status = http_status_list[8],
        .date = NULL, .server = NULL,
        .last_modified = NULL,
        .content_length = NULL,
        .content_type = NULL,
        .connection = NULL,
        .accept_ranges = NULL,
        .content_location = NULL
    };
    int retcode = 0;
    char filepath[BUFSIZE]; /* path to requested file */
    struct stat fstat; /* file status */

    read_from_socket(sd, client_header, BUFSIZE, server->timeout);
    parsed_header = parse_http_header(client_header);

    // check on parsed http status
    //TODO HTTP_RANGE_NOT_SATISFIABLE handeln
    switch (parsed_header.httpState) {
        case HTTP_STATUS_INTERNAL_SERVER_ERROR:
            safe_printf("%s\n", "internal server error");
        case HTTP_STATUS_BAD_REQUEST:
            safe_printf("%s\n", "bad request");
        case HTTP_STATUS_NOT_IMPLEMENTED:
            safe_printf("%s\n", "not implemented");
            response_header_data.status = http_status_list[9];
            create_response_header_string(response_header_data, server_header);
            return write_response_header(sd, server_header, server);
        default:
            break;
    }

    //TODO check CGI
    //TODO Partial Content
    if (parsed_header.isCGI) {

    }

    strcpy(filepath, server->root_dir);
    strcat(filepath, parsed_header.filename);
    //snprintf(filepath, BUFSIZE, "%s%s", server->root_dir, parsed_header.filename);
    safe_printf("%s\n", filepath);

    retcode = stat(filepath, &fstat);
    if (retcode) {
        perror("ERROR: stat");
    }

    // check on parsed http method
    if (strcmp(parsed_header.method, "GET") == 0) { /* GET method */
        safe_printf("%s\n", "GET");
        response_header_data.status = http_status_list[0];
        create_response_header(filepath, response_header_data, fstat);
        create_response_header_string(response_header_data, server_header);
        write_response_header(sd, server_header, server);
        return write_response_body(sd, filepath, server);
    } else { /* HEAD method */
        safe_printf("%s\n", "HEAD");
        response_header_data.status = http_status_list[0];
        create_response_header(filepath, response_header_data, fstat);
        create_response_header_string(response_header_data, server_header);
        return write_response_header(sd, server_header, server);
    }

    response_header_data.status = http_status_list[8];
    create_response_header_string(response_header_data, server_header);
    return write_response_header(sd, server_header, server);

    return retcode;
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
        perror("ERROR: server accept()");
        return -1;
    }

    pid = fork();
    if (pid == 0) {
        /* 
         * child process 
         */
        retcode = close(sd);
        if (retcode < 0) {
            perror("ERROR: child close()");
        } /* end if */
        retcode = handle_client(nsd, server, client);
        if (retcode < 0) {
            perror("ERROR: child handle_client()");
            exit(EXIT_FAILURE);
        } /* end if */
        // TODO: muss man hier sd vom Kind schließen?!
        exit(EXIT_SUCCESS);
    } else if (pid > 0) {
        /* 
         * parent process 
         */
        retcode = close(nsd);
        if (retcode < 0) {
            perror("ERROR: parent close()");
        } /* end if */
    } else {
        /* 
         * error while forking 
         */
        safe_printf("ERROR: fork()");
    }

    return nsd;
} /* end of accept_client */

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
    //init_logging_semaphore();

    // create the server socket
    socketDescriptor = create_server_socket(&my_opt);
    if (retcode < 0) {
        perror("ERROR: creating socket()");
        exit(retcode);
    } /* end if */

    // here, as an example, show how to interact with the
    // condition set by the signal handler above
    printf("[%d] Starting server '%s'...\n", getpid(), my_opt.progname);
    server_running = true;
    while (server_running) {
        // TODO: add error handling to accept_client
        retcode = accept_client(socketDescriptor, &my_opt);
        if (retcode < 0) {
            perror("ERROR: accepting clients()");
            exit(retcode);
        } /* end if */
    } /* end while */

    printf("[%d] Good Bye...", getpid());
    return retcode;
} /* end of main */