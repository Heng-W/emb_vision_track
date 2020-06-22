OBJ_DIR:=$(patsubst $(SRC_TOPDIR)%,$(OBJ_TOPDIR)%,$(shell pwd))
DEP_DIR:=$(patsubst $(SRC_TOPDIR)%,$(DEP_TOPDIR)%,$(shell pwd))

SUBDIRS?=$(shell ls -F |grep "/$ ")
SRCS?=$(wildcard *.cpp)
OBJS:=$(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS:=$(patsubst %.cpp,$(DEP_DIR)/%.dep,$(SRCS))

INCLUDE_DIR+=$(TOPDIR)/include
INCLUDE_FLAG:=$(addprefix -I,$(INCLUDE_DIR))


.PHONY:all $(SUBDIRS)

all: $(SUBDIRS) $(OBJ_DIR) $(OBJS)	

$(SUBDIRS):
	@echo
	make -C $@

$(OBJ_DIR) $(DEP_DIR):
	mkdir -p $@


-include $(DEPS)

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAG) -c -o $@ $<


ifeq ("$(wildcard $(DEP_DIR))", "")
$(DEP_DIR)/%.dep: %.cpp $(DEP_DIR)
else
$(DEP_DIR)/%.dep: %.cpp
endif
	@echo
	@echo "Creating $@ ..."
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAG) -E -MM $< | \
	sed -e 's|^\(.*\)\.o|$(OBJ_DIR)/\1\.o $(DEP_DIR)/\1\.dep|' > $@


