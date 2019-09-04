# adapted from https://stackoverflow.com/questions/48791883/best-practice-for-building-a-make-file/48793058#48793058

MAKEFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
BASE_DIR_TRAILING_SLASH := $(dir $(MAKEFILE_PATH))
export BASE_DIR := $(BASE_DIR_TRAILING_SLASH:%/=%)
BUILDSYS_DIR := $(BASE_DIR)/buildsys
export SRC_DIRS := $(wildcard src/*/.) src/.
include $(BUILDSYS_DIR)/path.mk
include $(BUILDSYS_DIR)/compiler_vars.mk
exes := # Executables to build.

#exes += my_benchmark
#objects.my_benchmark = my_benchmark.o filter.o utils.o compact_encoder.o modular_encoder.o direct_encoder.o encoder.o constraints.o vis_info.o xta_printer.o xml_printer.o timed_automata.o

exes += rcll_perception
objects.rcll_perception = rcll_perception.o utap_trace_parser.o filter.o utils.o compact_encoder.o modular_encoder.o direct_encoder.o encoder.o constraints.o vis_info.o xta_printer.o xml_printer.o timed_automata.o
all : $(SRC_DIRS) ${exes:%=${BUILD_DIR}/%} # Build all exectuables.

$(SRC_DIRS):
	$(info $(BUILDSYS_DIR))
	$(info $(BASE_DIR))
	$(MAKE) -C $@

.SECONDEXPANSION:
${exes:%=${BUILD_DIR}/%} : ${BUILD_DIR}/% : $$(addprefix ${LIB_DIR}/,$${objects.$$*}) Makefile | ${BUILD_DIR}
	$(strip ${LINK.EXE})
	$(info $(strip ${LINK.EXE}))

# Create the build directory on demand.
${BUILD_DIR} :
	mkdir $@


clean :
	rm -rf ${LIB_DIR}

check:
	find ${BASE_DIR}/ -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' -o -iname '*.hpp'     | xargs clang-format -style=LLVM -i -fallback-style=none

.PHONY : check clean all $(SRC_DIRS)
