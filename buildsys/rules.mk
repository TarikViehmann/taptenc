# see: http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/
ifeq ($(BASE_DIR),)
$(error BASE_DIR $(BASE_DIR) is not set)
endif
ifeq ($(BUILDSYS_DIR),)
	BUILDSYS_DIR := $(BASE_DIR)/buildsys
endif
include $(BUILDSYS_DIR)/path.mk
include $(BUILDSYS_DIR)/compiler_vars.mk

OBJ_FILES := $(SRCS:%.cpp=$(LIB_DIR)/%.o)
DEP_FILES := $(SRCS:%.cpp=$(DEP_DIR)/%.d)
.PHONY: all
all: $(OBJ_FILES)

$(OBJ_FILES) : ${LIB_DIR}/%.o: %.cpp $(DEP_DIR)/%.d | $(DEP_DIR)
	$(COMPILE.CXX)
	$(POSTCOMPILE)

$(DEP_DIR): ; @mkdir -p $@

$(DEP_FILES):
include $(wildcard $(DEP_FILES))
