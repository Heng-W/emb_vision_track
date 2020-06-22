
OBJS=$(foreach n,$(shell find $(OBJ_TOPDIR) -type d),$(wildcard $(n)/*.o))


$(BIN): $(OBJS)
	@echo
	@echo "Create execute file"
	$(CXX) $(CXXFLAGS) $(LINKFLAGS) -o $@ $^
	@echo
	@echo "Build successfully!"




