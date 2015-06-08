/*
 * socket_info.h
 *
 * $Id: socket_info.h,v 1.2 2004/12/29 00:37:29 ralf Exp $
 *
 */

#ifndef _SOCKET_INFO_H
#define _SOCKET_INFO_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

struct socket_info {
  char name[100];
  char addr[20];
  int port;
};

void get_socket_info(struct sockaddr_in from_sa, struct socket_info *si);

int get_socket_name(int fd, struct socket_info *si);

int get_socket_peer(int fd, struct socket_info *si);

#endif
