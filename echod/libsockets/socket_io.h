/*
 * socket_io.h
 *
 * $Id: socket_io.h,v 1.2 2004/12/29 00:37:29 ralf Exp $
 *
 */

#ifndef _SOCKET_IO_H
#define _SOCKET_IO_H

#define SOCKET_TIMEOUT  -2

int select_socket_fd (int fd, int maxtime, int writep);
int read_from_socket (int fd, char *buf, int len, int timeout);
int write_to_socket (int fd, char *buf, int len, int timeout);

#endif
