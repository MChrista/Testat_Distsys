#=============================================================================
#
# Makefile - Common Definitions
#
#-----------------------------------------------------------------------------
#
# DHBW Ravensburg - Campus Friedrichshafen
#
# Vorlesung Systemnahe Programmierung / Verteilte Systeme
#
#-----------------------------------------------------------------------------
#
# Author: Ralf Reutemann
#
#=============================================================================


SHELL       = /bin/sh
CC          = gcc
LD          = ld
CFLAGS      = -Wall -Werror -pedantic -std=gnu99
CFLAGS     += -Wunreachable-code -Wswitch-default
CFLAGS     += -O2


