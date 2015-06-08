#
# Makefile - libsockets 
#
# $Id: Makefile,v 1.1 2004/12/29 00:04:59 ralf Exp $
#

CC = gcc
CFLAGS = -g -O2 -Wall -pedantic -fPIC

OBJ = connect_tcp.o passive_tcp.o socket_info.o socket_io.o

SRC = connect_tcp.c passive_tcp.c socket_info.c socket_io.c

TARGETS = libsockets.a libsockets.so

.phony:
all: $(TARGETS)

libsockets.a : $(OBJ)
	rm -f $@
	ar qcv $@ $(OBJ)
	ranlib $@

libsockets.so : $(OBJ)
	$(CC) -shared -o $@ $(OBJ)

.phony:
depend:
	makedepend $(SRC)

.phony:
clean:
	rm -f *.o $(LIB)

