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
#-----------------------------------------------------------------------------
#
# Author: Ralf Reutemann
#
#=============================================================================


include ../common_defs.mk

#-----------------------------------------------------------------------------
# Configure source directory
#-----------------------------------------------------------------------------
SRC_DIR     := .
CFLAGS      += -I$(SRC_DIR)
SRCS        := $(wildcard $(SRC_DIR)/*.c)

#-----------------------------------------------------------------------------
# Configure OS/Architecture-specific build directory and create if necessary
#-----------------------------------------------------------------------------
OS          := $(shell uname -s)
ARCH        := $(shell uname -m)
BUILD_DIR   := build/$(OS)_$(ARCH)
OBJ_DIR     := $(BUILD_DIR)
foo         := $(shell test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR))
foo         := $(shell test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR))
OBJS        := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
#-----------------------------------------------------------------------------

TARGETS = $(BUILD_DIR)/libsockets.a

.PHONY:
all: $(TARGETS)

$(BUILD_DIR)/libsockets.a : $(OBJS)
	@echo AR $@
	@rm -f $@
	@ar qcv $@ $(OBJS)
	@ranlib $@

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@echo CC $<
	@$(CC) $(CFLAGS) -I$(SRC_DIR) -o $(OBJ_DIR)/$*.o -c $<

.PHONY:
clean:
	rm -f $(BUILD_DIR)/*.o $(TARGETS)

