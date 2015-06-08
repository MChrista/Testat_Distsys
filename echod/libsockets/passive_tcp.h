/*
 * passive_tcp.h
 *
 * $Id: passive_tcp.h,v 1.2 2004/12/29 00:37:29 ralf Exp $
 *
 */

#ifndef _PASSIVE_TCP_H
#define _PASSIVE_TCP_H

unsigned short get_port_from_name(const char *service);

int passive_tcp(unsigned short port, int qlen);

#endif
