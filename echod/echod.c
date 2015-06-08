/*
 * echod.c
 *
 * $Id: echod.c,v 1.2 2004/12/29 20:18:33 ralf Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#include "libsockets/passive_tcp.h"
#include "libsockets/socket_io.h"


#define BUFFER_SIZE       8192
#define MAX_NUM_THREADS      3
#define FALSE   0
#define TRUE    1


struct client_data {
  int sd;
  char name[100];
  char addr[20];
  int port;
};


struct thread_info {
  int             num_active;
  int             max_active;
  pthread_cond_t  thread_exit_cv;
  pthread_mutex_t mutex;
};


/*********************************************************************
 * Global variables
 *********************************************************************/

static int server_running;
struct thread_info slave_thread_info;


/*********************************************************************
 * Declaration of local functions
 *********************************************************************/

static int accept_clients(int sd);

static void *slave_thread(void *arg);

static void print_usage(char *progname);

static void get_client_data(struct sockaddr_in from_sa, struct client_data *ci);

void sig_handler(int sig);

/*
 *------------------------------------------------------------------------
 */


int
main(int argc, char *argv[])
{
  int sd;             /* socket descriptor */
  int port;           /* passive socket port */
  int retcode;        /* program return code */

  if (argc != 2) {
    fprintf(stderr, "Invalid number of arguments.\n\n");
    print_usage(argv[0]);
    return 1;
  } /* end if */

  port = atoi(argv[1]);

  sd = passive_tcp(port, 5);
  if (sd < 0) {
    fprintf(stderr, "ERROR: cannot create passive TCP connection.\n");
    return 1;
  } /* end if */

  /* (void) signal(SIGINT, sig_handler); */

  pthread_mutex_init(&slave_thread_info.mutex, NULL);
  pthread_cond_init(&slave_thread_info.thread_exit_cv, NULL);

  printf("Accepting client requests on port %d.\n", port);
  retcode = accept_clients(sd);

  pthread_mutex_destroy(&slave_thread_info.mutex);
  pthread_cond_destroy(&slave_thread_info.thread_exit_cv);

  return retcode;
} /* end of main */


static void
print_usage(char *s)
{
  char *p;

  p = strrchr(s, '/');
  if(p) {
    p++;
  } else {
    p = s;
  } /* end if */

  fprintf(stderr, "Usage: %s port\n", p);
} /* end of print_usage */


static int
accept_clients(int sd)
{
  int retcode = 0;                 /* function return value */
  int nsd;                         /* new socket descriptor for accept */
  struct sockaddr_in from_client;  /* socket address of connected client */
  unsigned int from_client_len;    /* size of socket address structure */
  struct client_data *from_info;   /* info for each connected client */
  pthread_t thread_id;
  pthread_attr_t thread_attr;

  pthread_attr_init(&thread_attr);
  pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
  server_running = TRUE;
  /*
   * Repeatedly call accept to receive the next request from a client,
   * and create a new slave process to handle the response.
   */
  while (server_running == TRUE) {
    /*
     * Have we exceeded our limit of active threads? If so, wait for
     * an active thread to finish.
     */
    pthread_mutex_lock(&slave_thread_info.mutex);
    while (slave_thread_info.num_active == MAX_NUM_THREADS) {
      pthread_cond_wait(&slave_thread_info.thread_exit_cv, &slave_thread_info.mutex);
    } /* end while */
    slave_thread_info.num_active++;
    pthread_mutex_unlock(&slave_thread_info.mutex);

    /*
     * Accept creates a new connected socket and allocates
     * a new file descriptor for the new socket, which is
     * returned to nsd.
     */
    from_client_len = sizeof from_client;
    nsd = accept(sd, (struct sockaddr *)&from_client, &from_client_len);
    if (nsd < 0) {
      if (errno == EINTR) {
	continue;
      } /* end if */
      perror("ERROR: server accept() ");
      server_running = FALSE;
      retcode = 1;
    } else {
      from_info = (struct client_data *)malloc(sizeof(struct client_data));
      if (from_info == NULL) {
	fprintf(stderr, "ERROR: malloc\n");
	server_running = FALSE;
	retcode = 1;
      } else {
        from_info->sd = nsd;
	get_client_data(from_client, from_info);
	pthread_create(&thread_id, &thread_attr, slave_thread, from_info);
      } /* end if */
    } /* end if */
  } /* end while */

  pthread_attr_destroy(&thread_attr);

  return retcode;
} /* end of accept_client */


static void *
slave_thread(void *arg)
{
  struct client_data *from_info;   /* identity of connected client */
  int bytes_read;                  /* number of bytes read from socket */
  int bytes_written;               /* number of bytes written to socket */
  char *buffer;                    /* buffer for socket read/write data */

  /* get connection data from argument pointer */
  from_info = (struct client_data *)(arg);

  buffer = (char *)malloc(BUFFER_SIZE);
  if (buffer == NULL) {
    fprintf(stderr, "ERROR: cannot allocate memory for buffer\n");
    exit(1);
  } /* end if */

  printf("Connection from host %s (%s), port %d.\n",
	 from_info->name, from_info->addr, from_info->port);

  do {
    bytes_read = read_from_socket(from_info->sd, buffer, BUFFER_SIZE, 100);
    if (bytes_read > 0) {
      bytes_written = write_to_socket(from_info->sd, buffer, bytes_read, 100);
    } /* end if */
  } while (bytes_read > 0);

  close(from_info->sd);

  printf("Connection closed for host %s (%s), port %d.\n",
	 from_info->name, from_info->addr, from_info->port);

  /*
   * Deallocate memory
   */
  free(buffer);
  free(from_info);

  pthread_mutex_lock(&slave_thread_info.mutex);
  slave_thread_info.num_active--;
  if (slave_thread_info.num_active == (MAX_NUM_THREADS - 1)) {
    pthread_cond_signal(&slave_thread_info.thread_exit_cv);
  } /* end if */
  pthread_mutex_unlock(&slave_thread_info.mutex);

  pthread_exit(NULL);
} /* end of slave_thread */


static void
get_client_data(struct sockaddr_in from_sa, struct client_data *ci)
{
  struct hostent *from_he;

  from_he = gethostbyaddr((char *)&from_sa.sin_addr,
			  sizeof(from_sa.sin_addr),
			  AF_INET);

  strncpy(ci->name,
	  (from_he) ? from_he->h_name : inet_ntoa(from_sa.sin_addr),
	  sizeof(ci->name));
  strncpy(ci->addr, inet_ntoa(from_sa.sin_addr), sizeof(ci->addr));
  ci->port = ntohs(from_sa.sin_port);
} /* end of get_client_data */


void
sig_handler(int sig)
{
  if (sig == SIGINT) {
    fprintf(stderr, "Server terminated due to keyboard interrupt.\n");
    server_running = FALSE;
  } /* end if */
} /* end of sig_handler */
