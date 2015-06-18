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
 * strcat for char pointer
 * @param   the first pointer
 * @param   the second pointer
 * @return  the concatenated char pointer
 */
char *scat(char *s,char *t)
{
    //TODO: remove this function
    char *p=malloc(strlen(s)+strlen(t)+1);
    int ptr = 0, temp = 0;               

    while(s[temp] != '\0'){      
        p[ptr++] = s[temp++];
    }
    temp = 0;
    while(t[temp] != '\0'){   
        p[ptr++] = t[temp++];
    }
    p[ptr] = '\0';
    return p;
} /* end of scat */

/**
 * creates as string from the header
 * @param   the http header
 * @return  the char pointer
 */
static char
*header_to_string(http_header_t header)
{
    //safe_printf("%s\n", header.status);
    char *response = "";

    // TODO: replace scat with strcat
    // status
    response = scat(scat(response, header.status), "\n");
    // date
    response = scat(scat(scat(response, http_header_field_list[0]), header.date), "\n");
    // server
    response = scat(scat(scat(response, http_header_field_list[1]), header.server), "\n");
    // last-modified /* already with /n because auf ctime call */
    response = scat(scat(response, http_header_field_list[2]), header.last_modified);
    // content-length
    response = scat(scat(scat(response, http_header_field_list[3]),header.content_length), "\n");
    // content-type
    response = scat(scat(scat(response, http_header_field_list[4]), header.content_type), "\n");
    // connection-close
    //response = scat(scat(scat(response, http_header_field_list[5]), header.connection), "\n");
    // close header
    response = scat(response, "\r\n\r\n");

    return response;
} /* end of header_to_string */

/**
 * creates the http response header
 * @param   the desired http status
 * @param   the file status
 * @param   the used protocol
 * @return  >0 in case of error
 */
static http_header_t
create_response_header(struct http_status_entry status, struct stat filestatus, char *protocol, char *filename) {
    http_header_t result; /* the header to return */
    
    time_t rawtime;       /* server time */
    struct tm * timeinfo; /* used for date field */
    char timeString [80]; /* the time as a string */

    /*
     * http status
     */
    // TODO: concatenate the http status
    // CAUTION: not every string is 0-terminated!
    result.status = "HTTP/1.1 200 OK";
     /*
    char *statusString = malloc(strlen(protocol) + strlen(status.text) + 1);
    strcpy(statusString,protocol);
    strcat(statusString,(char *)status.text);
    result.status = statusString;
    */

    /*
     * date
     */
    /* get time and format to rfc 2616 */
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime(timeString, 80, "%a, %d %b %Y %T %z %p.", timeinfo);
    result.date = timeString;
    
    /*
     * server
     */
    result.server = "TinyWeb Version 0.4.1";
    
    /*
     * last-modified
     */
    result.last_modified = ctime(&filestatus.st_mtime);
    
    /*
     * content-length
     */
    char filesize[256];
    sprintf(filesize, "%lld", (long long)filestatus.st_size);
    result.content_length = filesize;

    /*
     * content-type
     */
    http_content_type_t contentType;
    contentType = get_http_content_type(filename);
    result.content_type = get_http_content_type_str(contentType);
    
    /*
     * connection
     */
    //result.connection = "close";
    
    /*
     * accept-ranges
     */
    // TODO

    /*
     * content-location
     */
    // TODO: if(statuscode = 301) -> give location

    /*
     * range
     */
    // TODO 

    return result;
} /* end of create_response_header */

/**
 * creates the reply to client
 * @param   the socket descriptor
 * @param   the HTTP Method as char[]
 * @param   the requested file
 * @param   the program options
 * @return  >0 in case of error
 */
static int
return_http_file(int sd, parsed_http_header_t parsed_header, prog_options_t *server) {
    char *internalFilename;                  /* internal filename for check */
    char *reqFile = server->root_dir;        /* the resulting file path */
    int retcode;                             /* the return code */
    struct stat filestatus;                  /* file metadata */
    struct http_status_entry http_status;    /* the http status */ 
    http_header_t header;                    /* the freshly assembled header */
    char *reply = "Not yet implemented!\n";  /* the reply char pointer with default value */
    char *protocol = parsed_header.protocol; /* the request protocol */
    char *filename = parsed_header.filename; /* the requested file */
    char *type     = parsed_header.method;   /* the request method */
    int file;                                /* file descriptor of requested file */
    int chunkSize = 256;                     /* chunk size for write of message body */
    char chunk[chunkSize];                   /* chunk for write of message body */

    // default the status to 500
    http_status = http_status_list[8];

    // malloc to avoid really long filenames
    internalFilename = (char *) malloc(strlen(filename) + 1);
    if (internalFilename != NULL) {
        strcpy(internalFilename, filename);
    } else {
        err_print("cannot allocate memory");
        return EXIT_FAILURE;
    } /* end if */

    // path to folder + filename
    strcat(reqFile, internalFilename);

    retcode = stat(reqFile, &filestatus);
    if(retcode < 0) {
        // TODO: handle 'No such file or directory' with 404
        perror("ERROR: stat()");
        return -1;
    }

    // TODO: merge the if's below into one huge
    // check the requested file
    if(!(S_ISREG(filestatus.st_mode))) {
        /* requested file doesn't exist -> 404 */
        // TODO: this is probably dead code
        http_status = http_status_list[6];
        header = create_response_header(http_status, filestatus, protocol, filename);
        reply = header_to_string(header);
        return -1;
    } else if ((filestatus.st_mode & S_IFMT) == S_IROTH) {
        /* requested file is not for public -> 403 */
        http_status = http_status_list[5];
        create_response_header(http_status, filestatus, protocol, filename);
        return -1;
    } else if(S_ISDIR(filestatus.st_mode) && (((filestatus.st_mode & S_IFMT) == S_IXOTH))) {
        /* requested 'file' is a directory -> 301 */
        http_status = http_status_list[2];
        create_response_header(http_status, filestatus, protocol, filename);
        return -1;
    } /* end if */

    // determine the method type
    if (strncmp(type, "GET", sizeof("GET")) == 0) { 
        /* GET method */
        http_status = http_status_list[0];
        header = create_response_header(http_status, filestatus, protocol, filename);
        reply = header_to_string(header);
        //safe_printf(reply);
    } else if (strncmp(type, "HEAD", sizeof("HEAD")) == 0) { 
        /* HEAD method */
        http_status = http_status_list[0];
        header = create_response_header(http_status, filestatus, protocol, filename);
        reply = header_to_string(header);
        //safe_printf(reply);
    } else { 
        /* unsupported method -> 501 */
        http_status = http_status_list[9];
        create_response_header(http_status, filestatus, protocol, filename);
    } /* end if */

    // write header
    retcode = write(sd, reply, strlen(reply) - 1);
    if (retcode < 0) {
        perror("ERROR: write()");
        return -1;
    }

    // write file
    file = open(reqFile, O_RDONLY);
    if (file < 0) {
        perror("ERROR: fopen():");
    }

    while (read(file, &chunk, chunkSize)) {
        retcode = write(sd, chunk, chunkSize);
        if (retcode < 0) {
            perror("ERROR: write()");
            return -1;
        }
    }

    return retcode;
} /* end of return_http_file */

/**
 * Handle clients.
 * @param   the socket descriptor to read on
 * @param   the program options
 * @return  >0 in case of error
 */
static int
handle_client(int sd, prog_options_t *server) {
    // maybe define a HTTP_MAX_HEADER_SIZE to prevent DOS attacks
    // run till /n/r/n/r
    int BUFSIZE = 1000; /* buffer size */
    char buf[BUFSIZE]; /* buffer */
    int cc; /* character count */
    parsed_http_header_t parsed_header; /* parsed header */

    // read from client
    while ((cc = read(sd, buf, BUFSIZE)) > 0) {
        //TODO: Test maximal bufsize
        // parse the header
        parsed_header = parse_http_header(buf);

        return_http_file(sd, parsed_header, server);
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
accept_client(int sd, prog_options_t *server) {
    
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

    print_log("%s/n", "Hello World!");

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