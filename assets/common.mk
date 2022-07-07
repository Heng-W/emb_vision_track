

TARGET_DIR:=$(patsubst $(TOPDIR)/$(SRC_TOPDIR)%,%,$(shell pwd))
REL_DIR:=$(shell echo $(TARGET_DIR) | sed -r 's/\/[^\/]+/..\//g')
REL_DIR:=../$(REL_DIR)

OBJ_DIR:=$(REL_DIR)$(OBJ_TOPDIR)$(TARGET_DIR)
DEP_DIR:=$(REL_DIR)$(DEP_TOPDIR)$(TARGET_DIR)

INSTALL_INC_DIR:=$(INSTALL_DIR)/include$(TARGET_DIR)
TARGET_INSTALL_INCS:=$(patsubst %,$(INSTALL_INC_DIR)/%,$(INSTALL_INCS))


SUBDIRS?=$(shell ls -F | grep /$$ | grep -v "\.d/$$" | sed 's|/$$||g')
SRCS?=$(wildcard *.cpp)
OBJS:=$(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS:=$(patsubst %.cpp,$(DEP_DIR)/%.dep,$(SRCS))

ifeq ("$(wildcard $(OBJ_DIR))", "")
$(shell mkdir -p $(OBJ_DIR))
endif

ifeq ("$(wildcard $(DEP_DIR))", "")
$(shell mkdir -p $(DEP_DIR))
endif

# check subdir list
SUBDIR_LIST_FILE:=$(OBJ_DIR)/dir_list

ifeq ("$(wildcard $(SUBDIR_LIST_FILE))", "")
$(shell touch $(SUBDIR_LIST_FILE))
endif

ifneq ($(shell cat $(SUBDIR_LIST_FILE)), $(SUBDIRS))
$(shell echo $(SUBDIRS) > $(SUBDIR_LIST_FILE))
endif

# check obj list
OBJ_LIST_FILE:=$(OBJ_DIR)/list
OBJ_LIST:=$(patsubst %.cpp,%.o,$(SRCS))

ifeq ("$(wildcard $(OBJ_LIST_FILE))", "")
$(shell touch $(OBJ_LIST_FILE))
endif

ifneq ($(shell cat $(OBJ_LIST_FILE)), $(OBJ_LIST))
$(shell echo $(OBJ_LIST) > $(OBJ_LIST_FILE))
endif


# check dep list
RET:=$(shell bash $(REL_DIR)check_built_deps.sh "$(DEPS)")

INCLUDE_DIR+=$(REL_DIR)include $(EXTRA_INCLUDE_DIR)
INCLUDE_FLAG:=$(addprefix -I,$(INCLUDE_DIR))


.PHONY: all $(SUBDIRS) install

all: $(SUBDIRS) $(OBJS)

$(SUBDIRS):
	@echo
	$(MAKE) -C $@

-include $(DEPS)

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAG) -c -o $@ $<


$(DEP_DIR)/%.dep: %.cpp
	@echo "Build dependencies of $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDE_FLAG) -E -MM $< | \
	sed -e 's|^\(.*\)\.o|$(OBJ_DIR)/\1\.o $(DEP_DIR)/\1\.dep|' > $@


install: $(TARGET_INSTALL_INCS) 
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
		echo; \
	done

ifeq ("$(wildcard $(INSTALL_INC_DIR))", "")
$(INSTALL_INC_DIR)/%: % $(INSTALL_INC_DIR)
else
$(INSTALL_INC_DIR)/%: %
endif
	cp $< $@

$(INSTALL_INC_DIR):
	mkdir -p $@ 


