# adapted from https://stackoverflow.com/questions/48791883/best-practice-for-building-a-make-file/48793058#48793058

MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
BASE_DIR_TRAILING_SLASH := $(dir $(MAKEFILE_PATH))
export BASE_DIR := $(BASE_DIR_TRAILING_SLASH:%/=%)
BUILDSYS_DIR := $(BASE_DIR)/buildsys
export SRC_DIRS := $(wildcard src/*/.) src/.
WITH_UTAP := 1
include $(BUILDSYS_DIR)/path.mk
include $(BUILDSYS_DIR)/compiler_vars.mk
exes := # Executables to build.
SCRIPTS = $(notdir $(wildcard ${SCRIPT_DIR}/*.bash))

exes += rcll
exes += simple_test
exes += household
NON_EXEC_OBJS = $(filter-out ${exes:=.o},$(notdir $(wildcard ${LIB_DIR}/*.o)))
objects.simple_test = $(NON_EXEC_OBJS) simple_test.o
objects.rcll = $(NON_EXEC_OBJS) rcll.o
objects.household = $(NON_EXEC_OBJS) household.o

all : scripts $(SRC_DIRS) ${exes:%=${BUILD_DIR}/%} # Build all exectuables.

$(SRC_DIRS):
	$(MAKE) -C $@

.SECONDEXPANSION:
${exes:%=${BUILD_DIR}/%} : ${BUILD_DIR}/% : $$(addprefix ${LIB_DIR}/,$${objects.$$*}) Makefile | ${BUILD_DIR}
	$(strip ${LINK.EXE})
	#$(info $(strip ${LINK.EXE}))

# Create the build directory on demand.
${BUILD_DIR} :
	mkdir $@


clean :
	rm -rf ${LIB_DIR}

check:
	find ${BASE_DIR}/ -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' -o -iname '*.hpp'     | xargs clang-format -style=LLVM -i -fallback-style=none

scripts:
	@$(foreach var,$(SCRIPTS), ln -s -f $(SCRIPT_DIR)/$(var) $(BUILD_DIR)/$(var);)

.PHONY : check scripts clean all $(SRC_DIRS)
