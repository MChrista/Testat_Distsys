#**********************************************************************
# Berufsakademie Ravensburg - Praktikum Verteilte Kommunikationssysteme
#
# $Id: Makefile,v 1.3 2004/12/30 17:13:26 ralf Exp $
#
#**********************************************************************


SHELL		= /bin/sh

CC		= gcc
CFLAGS		= -Wall -g -O2 -pedantic
PS2PDF		= ps2pdf
A2PS		= a2ps
AOPT		= --line-numbers=1

SRC = echod.c

LIBS = libsockets/libsockets.a 

.PHONY : all
all : $(LIBS) echod

%.pdf : %.ps
	$(PS2PDF) $< $@

%.ps : %.c
	$(A2PS) $< $(AOPT) -o $@

libsockets/libsockets.a:
	$(MAKE) -C libsockets

echod : echod.o $(LIBS)
	$(CC) -o $@ echod.o $(LIBS) -lpthread


.PHONY: depend
depend:
	makedepend $(SRC)

.PHONY: clean
clean:
	rm -f *.o $(LIBS) echod

echod.o: libsockets/passive_tcp.h libsockets/socket_io.h
