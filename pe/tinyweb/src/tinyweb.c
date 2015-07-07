/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Ralf Reutemann
 *
 *===================================================================*/

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

// Must be true for the server accepting clients,
// otherwise, the server will terminate
static volatile sig_atomic_t server_running = false;

#define IS_ROOT_DIR(mode)   (S_ISDIR(mode) && ((S_IROTH || S_IXOTH) & (mode)))

static void
sig_handler(int sig) 
{
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
print_usage(const char *progname) 
{
    fprintf(stderr, "Usage: %s options\n%s%s%s%s", progname,
            "\t-d\tthe directory of web files\n",
            "\t-f\tthe logfile (if '-' or option not set; logging will be redirected to stdout\n",
            "\t-p\tthe port logging is redirected to stdout.for the server\n",
            "TIT12 Gruppe 7: Michael Christa, Florian Hink\n");
} /* end of print_usage */

static int
get_options(int argc, char *argv[], prog_options_t *opt) 
{
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
open_logfile(prog_options_t *opt) 
{
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
check_root_dir(prog_options_t *opt) 
{
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
install_signal_handlers(void) 
{
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
create_server_socket(prog_options_t *server) 
{
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

/**
 * write the response to client socket
 * @param   the client socket descriptor
 * @param   the response header
 * @param   the request method
 * @param   the filepath to requested file
 * @return  on error -1 is returned
 */
static int
write_response(int sd, char *header, char *method, char *filepath)
{
    int retcode;
    int file;               /* file descriptor of requested file */
    int chunkSize = 256;    /* chunk size for write of message body */
    //char chunk[chunkSize];  /* chunk for write of message body */

    /*
     * write header
     */
    retcode = write(sd, header, strlen(header));
    if (retcode < 0) {
        perror("ERROR: write()");
        return -1;
    } /* end if */

    if (strcmp(method, "HEAD") == 0) {
        return retcode;
    }

    //safe_printf("%s\n", filepath);

    /*
     * write file
     */
    file = open(filepath, O_RDONLY);
    if (file < 0) {
        perror("ERROR: open()");
        return -1;
    } /* end if */

    unsigned char chunk[chunkSize];
    struct stat fstat;      /* file status */
    retcode = stat(filepath, &fstat);
    size_t bytesWritten = 0;
    size_t bytesToWrite = fstat.st_size;

    while (bytesWritten != bytesToWrite) {
        size_t writtenThisTime;
        size_t readThisTime;

        readThisTime = read(file, chunk, chunkSize);

        if(readThisTime < 0) {
            return -1;
        }

        writtenThisTime = write(sd, chunk, readThisTime);
        if (writtenThisTime == -1) {
            return -1;
        }

        bytesWritten += writtenThisTime;
    }

    return retcode;
} /* end of write_response */

/**
 * creates the response header
 * @param   the http status
 * @param   the header pointer
 * @param   the request method
 * @param   the path to requested file
 * @return  on error -1 is returned
 */
static int
create_response_header(http_status_entry_t httpstat, char *header, char *method, char *filepath)
{
    int retcode = EXIT_SUCCESS;
    struct stat fstat; /* file status */

    //status line
    snprintf(header, 50, "%s %hu %s\r\n", "HTTP/1.1", httpstat.code, httpstat.text);

    // server
    char server[30];
    snprintf(server, 30, "%s%s\r\n", http_header_field_list[1], "Tinyweb 1.1");
    strcat(header, server);

    // time
    char timeString [80];
    char date [50];
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeString, 80, "%a, %d %b %Y %H:%M:%S", timeinfo);
    snprintf(date, 50, "%s%s\r\n", http_header_field_list[0], timeString);
    strcat(header, date);

    // connection
    char connection [30];
    snprintf(connection, 30, "%s%s\r\n",http_header_field_list[5], "keep-alive");
    strcat(header, connection);

    retcode = stat(filepath, &fstat);

    if(S_ISDIR(fstat.st_mode)) {
        printf("%s\n", "directory found!");
        char moved_permanently [50];
        strcat(filepath, "/");
        snprintf(moved_permanently, 50, "%s%s\r\n", http_header_field_list[7], filepath);
        strcat(header, moved_permanently);
    } else {
        //last-modified
        char last_modified [50];
        timeinfo = localtime(&fstat.st_mtime);
        strftime(timeString, 80, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
        snprintf(last_modified, 50, "%s%s\r\n", http_header_field_list[2], timeString);
        strcat(header, last_modified);

        // content-length
        char content_length [30];
        snprintf(content_length, 29, "%s%lld\r\n", http_header_field_list[3], (long long)fstat.st_size);
        strcat(header, content_length);

        // content-type
        char content_type [30];
        char *contenttypestr;
        http_content_type_t contenttype;
        contenttype = get_http_content_type(filepath);
        contenttypestr = get_http_content_type_str(contenttype);
        snprintf(content_type, 30, "%s%s\r\n", http_header_field_list[4], contenttypestr);
        strcat(header, content_type);
    }

    // close header
    strcat(header, "\r\n");
    //safe_printf(header);
    return retcode;
} /* end of create_response_header */

/**
 * differentiates between request methods
 * @param   the socket descriptor
 * @param   the http status
 * @param   the request method
 * @param   the path to requested file
 * @return  on error -1 is returned
 */
static int
create_response(int sd, http_status_entry_t httpstat, char *method, char *filepath) 
{
    char header[500];

    if (httpstat.code == http_status_list[2].code) {
        create_response_header(httpstat, header, method, filepath);
        strcat(filepath, "/status/301.html");
        return write_response(sd, header, method, filepath);
    }

    if (strcmp(method, "GET") == 0) { /* GET method */
        create_response_header(httpstat, header, method, filepath);
        return write_response(sd, header, method, filepath);
    } else { /* HEAD method */
        create_response_header(httpstat, header, method, filepath);
        return write_response(sd, header, method, filepath);
    }
} /* end of create_response */

/**
 * differentiates between response statuses
 * @param   the socket descriptor
 * @param   the parsed request header
 * @param   the program options
 * @return  on error -1 is returned
 */
static int
return_response(int sd, parsed_http_header_t parsed_header, prog_options_t *server)
{
    int retcode;        /* return code */
    char *filepath;     /* path to requested file */
    struct stat fstat;  /* file status */

    // path to folder + filename
    filepath = malloc(strlen(parsed_header.filename) + strlen(server->root_dir) + 1);
    if (filepath == NULL) {
        perror("ERROR: malloc()");
        return -1;
    }

    switch(parsed_header.httpState) {
        case HTTP_STATUS_INTERNAL_SERVER_ERROR:
            strcpy(filepath, server->root_dir);
            strcat(filepath, "/status/500.html");
            return create_response(sd, http_status_list[8], parsed_header.method, filepath);
        case HTTP_STATUS_BAD_REQUEST:
            strcpy(filepath, server->root_dir);
            strcat(filepath, "/status/400.html");
            return create_response(sd, http_status_list[4], parsed_header.method, filepath);
        case HTTP_STATUS_NOT_IMPLEMENTED:
            strcpy(filepath, server->root_dir);
            strcat(filepath, "/status/501.html");
            return create_response(sd, http_status_list[9], parsed_header.method, filepath);
        default:
            break;
    }

    strcpy(filepath, server->root_dir);
    strcat(filepath, parsed_header.filename);
    retcode = stat(filepath, &fstat);
    if ((retcode < 0) && ((errno == ENOENT) || (errno == ENOTDIR))) {
        strcpy(filepath, server->root_dir);
        strcat(filepath, "/status/404.html");
        return create_response(sd, http_status_list[6], parsed_header.method, filepath);
    } else if ((retcode < 0) && (errno == EACCES)) {
        // no access to file
    } else if (retcode < 0) {
        perror("ERROR: stat()");
    }

    if(S_ISDIR(fstat.st_mode)) {
        return create_response(sd, http_status_list[2], parsed_header.method, filepath);
    }

    // TODO check for 301 und 304

    return create_response(sd, http_status_list[0], parsed_header.method, filepath);
} /* end of return_response */

/**
 * Handle clients.
 * @param   the socket descriptor to read on
 * @param   the program options
 * @return  on error -1 is returned
 */
static int
handle_client(int sd, prog_options_t *server) 
{
    // maybe define a HTTP_MAX_HEADER_SIZE to prevent DOS attacks
    // run till /n/r/n/r
    int BUFSIZE = 1000; /* buffer size */
    char buf[BUFSIZE]; /* buffer */
    int cc; /* character count */
    parsed_http_header_t parsed_header; /* parsed header */
    int retcode;

    // read from client
    while ((cc = read(sd, buf, BUFSIZE)) > 0) {
        //TODO: Test maximal bufsize
        // parse the header
        parsed_header = parse_http_header(buf);

        retcode = return_response(sd, parsed_header, server);
        if (retcode < 0) {
            parsed_header.httpState = HTTP_STATUS_INTERNAL_SERVER_ERROR;
            return_response(sd, parsed_header, server);
            return -1;
        }
    }
    if (cc < 0) { /* error occured while reading */
        perror("ERROR: read()");
    } else if (cc == 0) { /* zero indicates end of file */
        //TODO handle connection closed
        //safe_printf("%s\n", "connection closed!");
    } /* end if */

    // TODO: write request to log file
    return 0;
} /* end of handle_client */

/**
 * Accept clients on the socket.
 * @param   the socket descriptor
 * @param   the program options
 * @return  a new socket descriptor
 */
static int
accept_client(int sd, prog_options_t *server) 
{
    
    signal(SIGCHLD, sig_handler);
    
    int nsd;                    /* new socket descriptor */
    pid_t pid;                  /* process id */
    int retcode;                /* return code */
    struct sockaddr_in client;  /* the input sockaddr */
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
        retcode = handle_client(nsd, server);
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
main(int argc, char *argv[]) 
{
    int retcode = EXIT_SUCCESS;
    int sd;
    prog_options_t my_opt;

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
    init_logging_semaphore();

    // create the socket
    sd = create_server_socket(&my_opt);
    if (sd < 0) {
        perror("ERROR: creating socket()");
    } /* end if */

    // here, as an example, show how to interact with the
    // condition set by the signal handler above
    printf("[%d] Starting server '%s'...\n", getpid(), my_opt.progname);
    server_running = true;
    while (server_running) {
        // TODO: add error handling to accept_client
        accept_client(sd, &my_opt);
    } /* end while */

    printf("[%d] Good Bye...", getpid());
    exit(retcode);
} /* end of main */