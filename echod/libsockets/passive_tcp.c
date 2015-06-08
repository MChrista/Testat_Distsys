/*
 * passive_tcp.c
 *
 * $Id: passive_tcp.c,v 1.4 2004/12/29 21:23:23 ralf Exp $
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "passive_tcp.h"


unsigned short
get_port_from_name(const char *service)
{
  struct servent  *pse;      /* pointer to service information entry */
  unsigned short port;

  /*
   * Map service name to port number
   */
  pse = getservbyname(service, "tcp");
  if (pse != 0) {
    port = ntohs((unsigned short)pse->s_port);
  } else {
    port = (unsigned short)atoi(service);
  } /* end if */

  if (port == 0) {
    fprintf(stderr, "ERROR: cannot get '%s' serivce entry\n", service);
  } /* end if */

  return port;
} /* end of get_port_from_name */


int
passive_tcp(unsigned short port, int qlen)
{
  int retcode;
  int sd;                    /* socket descriptor */
  struct protoent *ppe;      /* pointer to protocol information entry */
  struct sockaddr_in server;
  const int on = 1;          /* used to set socket option */


  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);

  /*
   * Get protocol information entry.
   */
  ppe = getprotobyname("tcp");
  if (ppe == 0) {
    perror("ERROR: server getprotobyname(\"tcp\")");
    return -1;
  } /* end if */

  /*
   * Create a socket.
   */
  sd = socket(PF_INET, SOCK_STREAM, ppe->p_proto);
  if (sd < 0) {
    perror("ERROR: server socket()");
    return -1;
  } /* end if */

  /*
   * Set socket options.
   */
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  
  /*
   * Bind the socket to the provided port.
   */
  retcode = bind(sd, (struct sockaddr *)&server, sizeof(server));
  if (retcode < 0) {
    perror("ERROR: server bind()");
    return -1;
  } /* end if */

  /*
   * Place the socket in passive mode.
   */
  retcode = listen (sd, qlen);
  if (retcode < 0) {
    perror("ERROR: server listen()");
    return -1;
  } /* end if */

  return sd;
} /* end of passive_tcp */
