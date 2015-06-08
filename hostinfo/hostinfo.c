/*===================================================================
 * DHBW Ravensburg - Campus Friedrichshafen
 *
 * Vorlesung Verteilte Systeme
 *
 * hostinfo.c - resolve host names using DNS lookup functions
 *
 * Created: 2014-05-13
 *
 *===================================================================*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>


int
resolve_hostname(char *host)
{
    struct hostent     *phe;        /* pointer to host information entry  */
    struct sockaddr_in  sin;        /* an Internet endpoint address       */
    struct in_addr      addr;
    struct addrinfo     hints;
    struct addrinfo    *result;
    struct addrinfo    *rp;
    int                 err;
    int                 sd;
    char                addrbuf[INET6_ADDRSTRLEN];
    char              **pp;


    /*---------------------------------------------------------------------
     * Call the obsolete function gethostbyname() to resolve the host name
     *---------------------------------------------------------------------*/
    printf("Using gethostbyname() to resolve host name '%s'\n", host);
    if((phe = gethostbyname(host)) == NULL) {
        fprintf(stderr, "Cannot resolve name '%s': %s\n", host, hstrerror(h_errno));
    } else {
        /* Copy the resolved IP address into our address structure.
         * Note that by convention, 'h_addr' is a synonym for 'h_addr_list[0]' */
        memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
        /* Print the so called official host name */
        printf("Resolved name: %s\n", phe->h_name);

        if(inet_ntop(AF_INET, &sin.sin_addr, addrbuf, INET_ADDRSTRLEN) == NULL) {
            fprintf(stderr, "Cannot convert IPv4 address to string\n");
            return EXIT_FAILURE;
        } /* end if */
        printf("Resolved address: %s\n\n", addrbuf);

        /* Iterate through all known aliases of the resolved host name */
        for(pp = phe->h_aliases; *pp != NULL; pp++) {
            printf("alias: %s\n", *pp);
        } /* end if */

        /* Iterate through all known IP adresses of the resolved host name */
        for(pp = phe->h_addr_list; *pp != NULL; pp++) {
            addr.s_addr = ((struct in_addr *)*pp)->s_addr;
            printf("  %s\n", inet_ntoa(addr));
        } /* end for */
    } /* end if */


    /*---------------------------------------------------------------------
     * Call getaddrinfo() to abtain a list of addresses that are associated
     * with the host name
     *---------------------------------------------------------------------*/
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;   /* Allows IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;         /* Allow any protocol */

    printf("\nUsing getaddrinfo() to resolve host name '%s'\n", host);
    if((err = getaddrinfo(host, "http", &hints, &result)) != 0) {
        fprintf(stderr, "Cannot resolve name '%s': %s\n", host, gai_strerror(err));
        return EXIT_FAILURE;
    } /* end if */

    /* Walk through the returned list and print each address structure in dotted
     * notation */
    for(rp = result; rp != NULL; rp = rp->ai_next) {
        if(inet_ntop(AF_INET, &rp->ai_addr, addrbuf, INET_ADDRSTRLEN) == NULL) {
            fprintf(stderr, "Cannot convert address to string\n");
            return EXIT_FAILURE;
        } /* end if */

        if(rp->ai_family == AF_INET) {
            struct in_addr *ap = &((struct sockaddr_in *)rp->ai_addr)->sin_addr;
            if(inet_ntop(rp->ai_family, ap, addrbuf, INET_ADDRSTRLEN) == NULL) {
                fprintf(stderr, "Cannot convert IPv4 address to string\n");
                return EXIT_FAILURE;
            } /* end if */
            printf("  %-25s  IPv4   ", addrbuf);
        } else if(rp->ai_family == AF_INET6) {
            struct in6_addr *ap = &((struct sockaddr_in6 *)rp->ai_addr)->sin6_addr;
            if(inet_ntop(rp->ai_family, ap, addrbuf, INET6_ADDRSTRLEN) == NULL) {
                fprintf(stderr, "Cannot convert IPv6 address to string\n");
                return EXIT_FAILURE;
            } /* end if */
            printf("  %-25s  IPv6   ", addrbuf);
        } else {
            printf("  Unknown\n");
        } /* end if */

        if((sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
            printf("socket: %s\n", strerror(errno));
            continue;
        } /* end if */

        if(connect(sd, rp->ai_addr, rp->ai_addrlen) == -1) {
            close(sd);
            printf("connect: %s\n", strerror(errno));
            continue;
        } /* end if */

        printf("OK\n");
        close(sd);
    } /* end for */

    freeaddrinfo(rp);

    return EXIT_SUCCESS;
} /* end of resolve_hostname */


int
main(int argc, char *argv[])
{
    int status;

    if(argc < 2) {
        fprintf(stderr, "Usage: %s hostname\n", argv[0]);
        status = EXIT_FAILURE;
    } else {
        status = resolve_hostname(argv[1]);
    } /* end if */

    exit(status);
} /* end of main */

