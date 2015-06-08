#=============================================================================
#
# Makefile
#
#-----------------------------------------------------------------------------
#
# DHBW Ravensburg - Campus Friedrichshafen
#
# Vorlesung Systemnahe Programmierung / Verteilte Systeme
#
#=============================================================================

CC          = gcc
LD          = ld
NASM        = nasm
NASMOPT     = -g -f elf -F dwarf
CFLAGS      = -Wall -g -O2
GLDFLAGS    = -melf_i386
PS2PDF      = ps2pdf
A2PS        = a2ps
AOPT        = --line-numbers=1


#-----------------------------------------------------------------------------
# Configure OS/Architecture-specific build directory and create if necessary
#-----------------------------------------------------------------------------
OS         := $(shell uname -s)
ARCH       := $(shell uname -m)
BUILD_DIR  := build/$(OS)_$(ARCH)
dummy      := $(shell test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR))
#-----------------------------------------------------------------------------

TARGETS = $(BUILD_DIR)/hostinfo

.PHONY: all
all: $(TARGETS)

$(BUILD_DIR)/hostinfo : hostinfo.c
	$(CC) $(CFLAGS) -o $@ $<

%.pdf : %.ps
	$(PS2PDF) $< $@

%.ps : %.asm
	$(A2PS) $< $(AOPT) -o $@


.PHONY: clean
clean:
	rm -f *.o *.ps $(TARGETS)


