
CXX:=arm-linux-g++
CXXFLAGS:=-std=c++11 -Wall 
LINKFLAGS:=-pthread -lm

OUTPUT_NAME:=emb_vision_track

TYPE?=Release
ifeq ($(TYPE), Debug)
CXXFLAGS+= -O -g
else ifeq ($(TYPE), Release)
CXXFLAGS+= -O2 -DNDEBUG
else
$(error TYPE invalid)
endif

ifneq ("$(MACRO)", "")
CXXFLAGS+=$(addprefix -D,$(MACRO))
endif


ifeq ($(TYPE), Debug)
BIN_NAME?=$(OUTPUT_NAME).d
else
BIN_NAME?=$(OUTPUT_NAME)
endif


ifeq ($(TYPE), Debug)
OBJ_DIR_NAME?=obj.d
DEP_DIR_NAME?=dep.d
else
OBJ_DIR_NAME?=obj
DEP_DIR_NAME?=dep
endif

TOPDIR:=$(shell pwd)
SRC_TOPDIR:=src
OBJ_TOPDIR:=build/$(OBJ_DIR_NAME)
DEP_TOPDIR:=build/$(DEP_DIR_NAME)
LIB_DIR:=lib
LOG_DIR:=logs
BIN_DIR:=bin
BIN:=$(BIN_DIR)/$(BIN_NAME)

export CXX CXXFLAGS TOPDIR SRC_TOPDIR OBJ_TOPDIR DEP_TOPDIR EXTRA_INCLUDE_DIR INSTALL_DIR


# Compile CXX project

.PHONY: all $(SRC_TOPDIR)
all: $(SRC_TOPDIR) $(BIN_DIR)

$(SRC_TOPDIR):
	$(MAKE) -C $@
	@echo
	@echo "Check object files..."
	@$(MAKE) -f Makefile.link BIN=$(BIN) LINKFLAGS="$(LINKFLAGS)" \
	LIB_DIR="$(LIB_DIR)" OBJ_DIR_NAME="$(OBJ_DIR_NAME)" \
	EXTRA_LIB_DIR="$(EXTRA_LIB_DIR)" EXTRA_LIB_NAME="$(EXTRA_LIB_NAME)"
	@echo

$(BIN_DIR):
	mkdir -p $@

.PHONY: clean exec test install 

clean:
	rm -rf build/ $(LOG_DIR)
	rm -f `find $(BIN_DIR) -type f | egrep -v "(.sh$$|.bat$$)"`
	
exec:
	$(BIN)

test:
	$(MAKE) -C $@


INSTALL_DIR:=$(abspath $(PREFIX))
INSTALL_LIB_DIR:=$(INSTALL_DIR)/lib
INSTALL_LIB:=$(INSTALL_LIB_DIR)/$(BIN_NAME)

install: $(INSTALL_LIB)
	$(MAKE) -C src $@
	@echo
	@echo "installed to: $(INSTALL_DIR)"
	@echo

ifeq ("$(wildcard $(INSTALL_LIB_DIR))", "")
$(INSTALL_LIB):$(BIN) $(INSTALL_LIB_DIR)
else
$(INSTALL_LIB):$(BIN)
endif
	cp $< $@

$(INSTALL_LIB_DIR):
	mkdir -p $@






