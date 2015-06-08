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


include common_defs.mk

#-----------------------------------------------------------------------------
# Configure source directory
#-----------------------------------------------------------------------------
SRC_DIR     := src
CFLAGS      += -I$(SRC_DIR)
SRCS        := $(wildcard $(SRC_DIR)/*.c)

#-----------------------------------------------------------------------------
# Configure OS/Architecture-specific build directory and create if necessary
#-----------------------------------------------------------------------------
OS          := $(shell uname -s)
ARCH        := $(shell uname -m)
BUILD_DIR   := build/$(OS)_$(ARCH)
OBJ_DIR     := $(BUILD_DIR)/obj
foo         := $(shell test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR))
foo         := $(shell test -d $(OBJ_DIR) || mkdir -p $(OBJ_DIR))
OBJS        := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
#-----------------------------------------------------------------------------

LIB_SOCK    := libsockets/$(BUILD_DIR)/libsockets.a
CFLAGS      += -Ilibsockets

#-----------------------------------------------------------------------------
# Configure debug build settings
#-----------------------------------------------------------------------------
DBG_OBJ_DIR := $(BUILD_DIR)/debug
DBG_OBJS    := $(patsubst $(SRC_DIR)/%.c,$(DBG_OBJ_DIR)/%.o,$(SRCS))
foo         := $(shell test -d $(DBG_OBJ_DIR) || mkdir -p $(DBG_OBJ_DIR))
LIB_DEBUG   := libdebug/$(BUILD_DIR)/libdebug.a
CFLAGS      += -Ilibdebug
DEBUG       := -g -DDEBUG
LWRAP       := -Wl,--wrap,malloc -Wl,--wrap,free
#-----------------------------------------------------------------------------

TARGETS = $(BUILD_DIR)/tinyweb
ifeq ($(OS), Linux)
TARGETS     += $(BUILD_DIR)/tinyweb_debug
endif


.PHONY: all
all: $(TARGETS)

$(BUILD_DIR)/tinyweb : $(OBJS) $(LIB_SOCK)
	@echo LD $@
	@$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIB_SOCK) -lpthread

$(BUILD_DIR)/tinyweb_debug : $(DBG_OBJS) $(LIB_SOCK) $(LIB_DEBUG)
	@echo LD $@
	@$(CC) $(CFLAGS) $(LWRAP) -o $@ $(OBJS) $(LIB_SOCK) $(LIB_DEBUG) -lpthread

$(LIB_SOCK):
	$(MAKE) -C libsockets

$(LIB_DEBUG):
	$(MAKE) -C libdebug

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@echo CC $<
	@$(CC) $(CFLAGS) -I$(SRC_DIR) -Ilibsockets -o $(OBJ_DIR)/$*.o -c $<

$(DBG_OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@echo CC $(DEBUG) $<
	@$(CC) $(CFLAGS) -I$(SRC_DIR) -Ilibsockets $(DEBUG) -o $(DBG_OBJ_DIR)/$*.o -c $<

.PHONY: clean
clean:
	$(MAKE) -C libsockets clean
	$(MAKE) -C libdebug clean
	rm -f $(TARGETS)
	rm -rf $(BUILD_DIR)

