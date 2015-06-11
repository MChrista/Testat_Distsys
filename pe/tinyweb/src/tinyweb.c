/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * Author:  Ralf Reutemann
 *
 *===================================================================*/
//
// TODO: Include your module header here
//


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

// Must be true for the server accepting clients,
// otherwise, the server will terminate
static volatile sig_atomic_t server_running = false;

#define IS_ROOT_DIR(mode)   (S_ISDIR(mode) && ((S_IROTH || S_IXOTH) & (mode)))

//
// TODO: Include your function header here
//

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


//
// TODO: Include your function header here
//

static void
print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s options\n%s%s%s%s", progname,
            "\t-d\tthe directory of web files\n",
            "\t-f\tthe logfile (if '-' or option not set; logging will be redirected to stdout\n",
            "\t-p\tthe port logging is redirected to stdout.for the server\n",
            "TIT12 Gruppe 7: Michael Christa, Florian Hink\n");
} /* end of print_usage */

//
// TODO: Include your function header here
//

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

/**
 * Write connection details to log.
 */
static int
write_log() {
    // TODO: stdout or log file?!
    return 0;
} /* end of write_log */

/**
 * creates the http response header
 * @param   the requested file
 * @return  >0 in case of error
 */
static int
create_response_header(const char *filename) {
    // TODO implement this!
    /*
    char *header = 
        "HTTP/1.1 200 OK\n"
        "Date: Thu, 19 Feb 2009 12:27:04 GMT\n"
        "Server: Apache/2.2.3\n"
        "Last-Modified: Wed, 18 Jun 2003 16:05:58 GMT\n"
        "Content-Length: 15\n"
        "Content-Type: text/html\n"
        "Connection: close\n"
        "Accept-Ranges: bytes\n"
        "Content-Location: /index.html\n"
     */

    return -1;
} /* end of create_response_header */

/**
 * creates the reply to client
 * @param   the socket descriptor
 * @param   the HTTP Method (1=GET, 0=HEAD)
 * @return  >0 in case of error
 */
static int
return_method(int sd, int type) {
    // TODO: fix this function
    int retcode;
    create_response_header(NULL);

    switch (type) {
        case 1: /* GET */
            break;
        case 0: /* HEAD */
            break;
        default: /* unsupported */
            break;
    }

    char *reply = "Hello World!";

    retcode = write(sd, reply, strlen(reply) - 1);
    if (retcode < 0) {
        perror("ERROR: write()");
        return -1;
    }
    safe_printf("run through\n");
    return retcode;
} /* end of return_method */

/**
 * Handle clients.
 * @param   the socket descriptor to read on
 * @return  >0 in case of error
 */
static int
handle_client(int sd) {
    // maybe define a HTTP_MAX_HEADER_SIZE to prevent DOS attacks
    // run till /n/r/n/r
    int BUFSIZE = 1000; /* buffer size */
    char buf[BUFSIZE]; /* buffer */
    int cc; /* character count */
    parsed_http_header parsed_header; /* parsed header */

    // read from client
    while ((cc = read(sd, buf, BUFSIZE)) > 0) {
        //TODO: Test maximal bufsize
        // parse the header
        parsed_header = parse_http_header(buf);

        // determine the method type
        if (strncmp(parsed_header.method, "GET", sizeof (parsed_header.method)) == 0) { /* GET method */
            //safe_printf("%s\n", "GET method called");
            return_method(sd, 1);
        } else if (strncmp(parsed_header.method, "HEAD", sizeof (parsed_header.method)) == 0) { /* HEAD method */
            //safe_printf("%s\n", "HEAD method called");
            return_method(sd, 0);
        } else { /* unsupported method */
            safe_printf("%s\n", "unsupported method called");
            return_method(sd, -1);
        } /* end if */
    }
    if (cc < 0) { /* error occured while reading */
        perror("ERROR: read()");
    } else if (cc == 0) { /* zero indicates end of file */
        //TODO handle connection closed
        //safe_printf("%s\n", "connection closed!");
    } /* end if */



    // write request to log file
    write_log();
    return 0;
} /* end of handle_client */

/**
 * Accept clients on the socket.
 * @param   the socket descriptor
 * @return  a new socket descriptor
 */
static int
accept_client(int sd) {
    signal(SIGCHLD, sig_handler);
    int nsd; /* new socket descriptor */
    struct sockaddr_in client; /* the input sockaddr */
    socklen_t client_len = sizeof (client); /* the length of it */
    pid_t pid; /* process id */
    int retcode; /* return code */

    /*
     * accept clients on the socket
     */
    nsd = accept(sd, (struct sockaddr *) &client, &client_len);
    if (nsd < 0) {
        perror("ERROR: server accept()");
        return -1;
    }

    pid = fork();
    if (pid == 0) { /* child process */
        retcode = close(sd);
        if (retcode < 0) {
            perror("ERROR: child close()");
        } /* end if */
        retcode = handle_client(nsd);
        if (retcode < 0) {
            perror("ERROR: child handle_client()");
        } /* end if */
        exit(EXIT_SUCCESS);
    } else if (pid > 0) { /* parent process */
        retcode = close(nsd);
        if (retcode < 0) {
            perror("ERROR: parent close()");
        } /* end if */
    } else { /* error while forking */
        // use our own thread-safe implemention of printf
        safe_printf("ERROR: fork()");
        //exit(EXIT_FAILURE);
    }

    return nsd;
} /* end of accept_client */

int
main(int argc, char *argv[]) {
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
    // TODO: check existence of log_file

    // create the socket
    sd = create_server_socket(&my_opt);
    if (sd < 0) {
        perror("ERROR: creating socket()");
    } /* end if */

    // TODO: start the server and handle clients...
    // here, as an example, show how to interact with the
    // condition set by the signal handler above
    printf("[%d] Starting server '%s'...\n", getpid(), my_opt.progname);
    server_running = true;
    while (server_running) {
        // pause();
        // start accepting clients TODO: add error handling to accept_client
        accept_client(sd);
    } /* end while */

    printf("[%d] Good Bye...", getpid());
    exit(retcode);
} /* end of main */