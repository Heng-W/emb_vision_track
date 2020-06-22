CROSS_COMPILE?=arm-linux-
#CC=$(CROSS_COMPILE)gcc
#CFLAGS=-std=c99 -Wall -O2
CXX=$(CROSS_COMPILE)g++
CXXFLAGS=-std=c++11 -Wall -O2
LINKFLAGS=-pthread

BIN_NAME:=out
TOPDIR:=$(shell pwd)
SRC_TOPDIR:=$(TOPDIR)/src
OBJ_TOPDIR:=$(TOPDIR)/build/obj
DEP_TOPDIR:=$(TOPDIR)/build/dep
BIN_DIR:=$(TOPDIR)/build/bin
BIN:=$(BIN_DIR)/$(BIN_NAME)

export CXX CXXFLAGS TOPDIR SRC_TOPDIR OBJ_TOPDIR DEP_TOPDIR

LIB_DIRS:=$(shell find lib -type d)
LIB_NAMES:=$(notdir $(foreach n,$(LIB_DIRS),$(wildcard $(n)/*.so)))

LIB_DIR_FLAG:=$(addprefix -L $(TOPDIR)/,$(LIB_DIRS))
LIB_NAME_FLAG:=$(patsubst lib%.so,-l%,$(LIB_NAMES))
LINKFLAGS+=$(LIB_DIR_FLAG) $(LIB_NAME_FLAG)



# Compile C++ project

.PHONY: all $(SRC_TOPDIR) $(BIN)
all: $(SRC_TOPDIR) $(BIN_DIR) $(BIN)

$(SRC_TOPDIR):
	@echo
	make -C $@

$(BIN_DIR):
	mkdir -p $@

$(BIN):
	@echo
	@echo "Check object files..."	
	make -C build/ BIN=$(BIN) LINKFLAGS="$(LINKFLAGS)"
	@echo


.PHONY: exec clean

exec:
	$(BIN)

clean:
	rm -rf $(OBJ_TOPDIR) $(DEP_TOPDIR) $(BIN_DIR)



# kernel modules

modules modules_clean:
	@echo	
	make -C drivers/ $@

cleanall:
	make clean
	make modules_clean

